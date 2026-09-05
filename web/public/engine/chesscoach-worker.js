// Worker wrapper so the coach can drive our WASM engine with the same UCI
// message contract it uses for Stockfish: post a command string, receive
// one text line per message.
//
// No top-level await: with `await createModule()` at module scope the worker
// stays suspended until the WASM instantiates, so onmessage is not registered
// yet and the first `uci` command (posted right after new Worker) is dropped,
// leaving the engine idle forever. Instead we register the handler
// synchronously and buffer commands until the module is ready.
import createModule from './chesscoach.js';

let uci = null;
const pending = [];

function run(data) {
    const res = uci(String(data));
    if (!res) return;
    for (const line of res.split('\n'))
        if (line) self.postMessage(line);
}

self.onmessage = (e) => {
    if (uci) run(e.data);
    else pending.push(e.data);
};

createModule().then((mod) => {
    uci = mod.cwrap('uci_command', 'string', ['string']);
    for (const d of pending) run(d);
    pending.length = 0;
});
