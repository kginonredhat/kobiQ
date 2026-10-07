import Meta from 'gi://Meta';
import Gio from 'gi://Gio';
import GLib from 'gi://GLib';
import {Extension} from 'resource:///org/gnome/shell/extensions/extension.js';

const KOBIQ_BUS_NAME = 'org.kobiq.Application';
const KOBIQ_OBJECT_PATH = '/org/kobiq/Clipboard';
const KOBIQ_IFACE = 'org.kobiq.Clipboard';

const MAX_TEXT_BYTES = 10 * 1024 * 1024;
const READ_TIMEOUT_MS = 5000;
const READ_CHUNK_BYTES = 64 * 1024;
const SETTLE_MS = 150;

const TEXT_MIME_TYPES = [
  'text/plain;charset=utf-8',
  'UTF8_STRING',
  'text/plain',
  'STRING',
];

export default class KobiQClipboardMonitorExtension extends Extension {
  enable() {
    this._destroyed = false;
    this._clipboardTimeout = null;
    this._readCancellable = null;
    this._readTimeoutId = null;

    this._selection = global.display.get_selection();
    this._ownerChangedId = this._selection.connect(
      'owner-changed',
      this._onOwnerChanged.bind(this)
    );
  }

  disable() {
    this._destroyed = true;
    if (this._clipboardTimeout) {
      GLib.source_remove(this._clipboardTimeout);
      this._clipboardTimeout = null;
    }
    this._cancelRead();
    if (this._ownerChangedId && this._selection) {
      this._selection.disconnect(this._ownerChangedId);
      this._ownerChangedId = null;
    }
    this._selection = null;
  }

  _onOwnerChanged(_selection, selectionType, selectionSource) {
    if (selectionType !== Meta.SelectionType.SELECTION_CLIPBOARD)
      return;

    if (this._clipboardTimeout) {
      GLib.source_remove(this._clipboardTimeout);
      this._clipboardTimeout = null;
    }
    this._cancelRead();

    // Brief settle time so the new owner can advertise MIME types.
    this._clipboardTimeout = GLib.timeout_add(
      GLib.PRIORITY_DEFAULT, SETTLE_MS, () => {
        this._clipboardTimeout = null;
        this._readClipboard(selectionSource);
        return GLib.SOURCE_REMOVE;
      }
    );
  }

  _readClipboard(source) {
    if (!source)
      return;
    const offered = source.get_mimetypes();
    const textTypes = TEXT_MIME_TYPES.filter(t => offered.includes(t));
    if (textTypes.length > 0)
      this._readText(source, textTypes);
    else if (offered.some(t => t.startsWith('image/')))
      this._sendToKobiQ('image', '');
  }

  _readText(source, textTypes) {
    const cancellable = new Gio.Cancellable();
    this._readCancellable = cancellable;
    this._readTimeoutId = GLib.timeout_add(
      GLib.PRIORITY_DEFAULT, READ_TIMEOUT_MS, () => {
        this._readTimeoutId = null;
        this._readCancellable = null;
        cancellable.cancel();
        return GLib.SOURCE_REMOVE;
      }
    );

    const done = data => {
      if (cancellable.is_cancelled() || this._destroyed)
        return;
      this._endRead();
      if (data === null)
        return;
      if (data.length === 0) {
        if (textTypes.length > 1)
          this._readText(source, textTypes.slice(1));
        return;
      }
      if (data[data.length - 1] === 0)
        data = data.subarray(0, -1);
      const text = new TextDecoder().decode(data);
      if (text)
        this._sendToKobiQ('text', text);
    };

    source.read_async(textTypes[0], cancellable, (src, result) => {
      let stream;
      try {
        stream = src.read_finish(result);
      } catch (e) {
        console.debug(`kobiQ monitor: clipboard read failed: ${e.message}`);
        done(null);
        return;
      }
      this._readChunks(stream, cancellable,
        Gio.MemoryOutputStream.new_resizable(), done);
    });
  }

  _readChunks(stream, cancellable, into, done) {
    stream.read_bytes_async(READ_CHUNK_BYTES, GLib.PRIORITY_DEFAULT,
      cancellable, (src, result) => {
        let bytes = null;
        try {
          bytes = src.read_bytes_finish(result);
        } catch (e) {
          console.debug(`kobiQ monitor: clipboard read failed: ${e.message}`);
        }
        const size = bytes ? bytes.get_size() : 0;
        if (bytes !== null && size > 0 &&
            into.get_data_size() + size <= MAX_TEXT_BYTES) {
          into.write_bytes(bytes, null);
          this._readChunks(src, cancellable, into, done);
          return;
        }
        try {
          src.close(null);
        } catch (e) {
          console.debug(`kobiQ monitor: closing read failed: ${e.message}`);
        }
        into.close(null);
        done(bytes === null || size > 0
          ? null : into.steal_as_bytes().get_data() ?? new Uint8Array());
      });
  }

  _cancelRead() {
    const cancellable = this._readCancellable;
    this._endRead();
    cancellable?.cancel();
  }

  _endRead() {
    if (this._readTimeoutId) {
      GLib.source_remove(this._readTimeoutId);
      this._readTimeoutId = null;
    }
    this._readCancellable = null;
  }

  _sendToKobiQ(contentType, content) {
    Gio.DBus.session.call(
      KOBIQ_BUS_NAME,
      KOBIQ_OBJECT_PATH,
      KOBIQ_IFACE,
      'NewEntry',
      new GLib.Variant('(ss)', [contentType, content]),
      null,
      Gio.DBusCallFlags.NO_AUTO_START,
      -1,
      null,
      (connection, result) => {
        try {
          connection.call_finish(result);
        } catch (e) {
          console.debug(`kobiQ monitor: NewEntry not delivered: ${e.message}`);
        }
      }
    );
  }
}
