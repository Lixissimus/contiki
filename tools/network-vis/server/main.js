const { exec } = require('child_process');
const { spawn } = require('child_process');
const WebSocket = require('ws');
const now = require('performance-now');

const wss = new WebSocket.Server({ port: 8001 });
let ws;
wss.on('connection', _ws => {
  ws = _ws;
  console.log("Client connected!");
  const children = [];

  ws.on('message', message => {
    console.log('received: %s', message);
    const parsed = JSON.parse(message);

    switch (parsed.type) {
      case "connect":
        connectToCommServer(parsed.ip);
        break;
      case "request-sync":
        ws.send(JSON.stringify({
          type: "sync-1",
          timestamp: now()
        }));
        break;
      case "sync-2":
        let t = now();
        ws.send(JSON.stringify({
          type: "sync-3",
          t1: parsed.t1,
          t2: t - parsed.timestamp
        }));
        break;
      default:
        console.log("Received unknown message type");
    }
  });

  ws.on('close', () => {
    console.log("Client disconnected!");
    console.log('Killing child processes.');
    children.forEach(child => {
      child.kill();
    });
    console.log("Waiting for new client...");
  });

  exec('ls /dev | xargs -n 1 basename | grep ttyUSB', (err, stdout, stderr) => {
    if (err) {
      // node couldn't execute the command
      return;
    }

    stdout.split('\n').forEach(element => {
      if (element !== "") {
        console.log(element);
        const child = spawn('../../sky/serialdump-linux', ['-b115200', '/dev/' + element]);
        children.push(child);
        let rest = "";
        child.stdout.on('data', chunk => {
          // data from standard output is here as buffers
          const str = chunk.toString();
          let lines = str.split('\n');
          
          // sometimes the output is split inbetween lines, so we merge them back together
          if (rest != "") {
            lines[0] = rest + lines[0];
            rest = "";
          }
          if (!str.endsWith("\n")) {
            rest = lines.pop();
          }

          lines.forEach(line => {
            if (line.startsWith('#')) {
              const command = { type: "message" };
              const tokens = line.split(" ");
              switch (tokens[0]) {
                case "#H":
                  /* highlight command: #H <id> */
                  command.name = "H";
                  command.id = tokens[1];
                  break;
                case "#L":
                  /* line command: #L <src> <dst> */
                  command.name = "L";
                  command.src = tokens[1];
                  command.dst = tokens[2];
                  break;
                case "#R":
                  /* rank command: #R <id> <rank> */
                  command.name = "R";
                  command.id = tokens[1];
                  command.rank = tokens[2];
                  break;
                case "#N":
                  /* neighbor command: #N <id> <nbrId> */
                  command.name = "N";
                  command.id = tokens[1];
                  command.nbrId = tokens[2];
                  break;
                case "#P":
                  /* packet command: #P <from> <to> <mod 0|1> <seqNum> */
                  command.name = "P";
                  command.from = tokens[1];
                  command.to = tokens[2];
                  command.seqNum = tokens[4];
                  if (parseInt(tokens[3])) {
                    // received
                    command.mod = "rec";
                    command.latency = packetReceived(command.from, command.to, command.seqNum);
                  } else {
                    // sent
                    packetSent(command.from, command.to, command.seqNum);
                    command.mod = "sent";
                  }
                  break;
                case "#DR":
                  /* delivery ratio command: #DR <id> <from> <rec> <exp> */
                  command.name = "DR";
                  command.id = tokens[1];
                  command.from = tokens[2];
                  command.rec = tokens[3];
                  command.exp = tokens[4];
                  break;
                case "#DC":
                  /* duty cycle command: #DC <id> <enumerator> <divisor> */
                  command.name = "DC";
                  command.id = tokens[1];
                  command.dcOn = tokens[2];
                  command.total = tokens[3];
                default:
                  break;
              }
              command.clockDelta = 0;
              ws.send(JSON.stringify(command));
            } else if (line !== "") {
              console.log(line);
            }
          });
        });
      }
    });
  });
});

const sentTimes = {};

function packetSent(from, to, seqNum) {
  let t = now();
  sentTimes[`${from}-${to}-${seqNum}`] = t;
  console.log("send time:", t);
  return t;
}

function receivedPacketSent(from, to, seqNum, timestamp) {
  let t = timestamp - currentClockDelta();
  sentTimes[`${from}-${to}-${seqNum}`] = t;

  console.log("curr time:", now());
  console.log("write:", `${from}-${to}-${seqNum}`, t, timestamp);

  return t;
}

function packetReceived(_from, _to, seqNum) {
  let t = now();
  let from = parseInt(_from);
  let to = parseInt(_to);
  if (isNaN(from) || isNaN(to) || !sentTimes[`${from}-${to}-${seqNum}`]) {
    return -1;
  }

  const lat =  t - sentTimes[`${from}-${to}-${seqNum}`];
  delete sentTimes[`${from}-${to}-${seqNum}`];

  return lat;
}

const deltas = [];
let commConnection;
function connectToCommServer(ip) {
  commConnection = new WebSocket(`ws://${ip}:8001`);
  commConnection.on('open', () => {
    console.log("CommConnection open");
    startClockSync();
  });

  commConnection.on('close', () => {
    console.log("CommConnection closed");
    clearInterval(syncInterval);
  });

  commConnection.on('message', message => {
    const parsed = JSON.parse(message);
    switch (parsed.type) {
      case "message":
        if (parsed.name === "P" && parsed.mod === "sent") {
          // received message with timestamp, apply clock delta
          console.log("Received packet sent, calculate sent time with clock delta");
          parsed.timestamp = receivedPacketSent(parsed.from, parsed.to, parsed.seqNum, parsed.timestamp);
        }
        // forward message
        ws.send(JSON.stringify(parsed));
        break;
      case "sync-1":
        let t = now();
        commConnection.send(JSON.stringify({
          type: "sync-2",
          t1: t - parsed.timestamp,
          timestamp: t
        }));
        break;
      case "sync-3":
        let delta = (parsed.t2 - parsed.t1) / 2;
        if (deltas.length >= 5) {
          deltas.shift();
        }
        deltas.push(delta);
        console.log("deltas:", deltas);
        console.log("New delta:", delta);
        console.log("Current delta:", currentClockDelta());
        break;
      default:
        console.log("Unknown message type");
    }
  });
}

function currentClockDelta() {
  if (deltas.length === 0) {
    return -1;
  }

  let ret = deltas[0];
  deltas.forEach(d => {
    if (d < ret) {
      ret = d;
    }
  });
  
  // our time plus ret equals remote time
  return ret;
}

let syncInterval;
function startClockSync() {
  syncInterval = setInterval(initiateClockSync, 5000);
}

function initiateClockSync() {
  commConnection.send(JSON.stringify({
    type: "request-sync"
  }));
}

console.log("Waiting for connection...");
