const { exec } = require('child_process');
const { spawn } = require('child_process');
const WebSocket = require('ws');

const wss = new WebSocket.Server({ port: 8001 });
wss.on('connection', ws => {
  console.log("Client connected!");
  const children = [];

  ws.on('message', message => {
    console.log('received: %s', message);
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
              const command = {};
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
                  command.mod = parseInt(tokens[3]) ? "rec" : "sent";
                  command.seqNum = tokens[4];
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

console.log("Waiting for connection...");
