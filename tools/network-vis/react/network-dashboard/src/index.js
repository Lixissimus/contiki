import React from 'react';
import ReactDOM from 'react-dom';
import { Flex, Box } from 'reflexbox';

import * as _ from 'lodash'

import './index.css';

import Tile from './Tile';
import Network from './Network';
import Histogram from './Histogram';
import History from './History';


class App extends React.Component {
  constructor(props) {
    super(props);
    
    this.connection = null;
    
    this.networkComponent = null;

    this.setupCommunication();

    this.state = {
      latencies: [],
      nodes: [],
      links: [],
      ipHops: [],
      packets: []
    };
  }

  setupCommunication() {
    // const connectButton = d3.select("#connect-button");
    // const ipField = d3.select("#ip-field");

    // connectButton.on("click", () => {
    //   const ip = ipField.property("value");
    //   connection.send(JSON.stringify({
    //     type: "connect",
    //     ip: ip
    //   }));
    // });

    this.connection = new WebSocket('ws://127.0.0.1:8001');

    this.connection.onopen = () => {
      console.log('Connection open!');
    }

    this.connection.onerror = () => {
      console.log('Error occured!');
    }

    this.connection.onmessage = message => {
      const command = JSON.parse(message.data);
      switch (command.name) {
        case 'H':
          /* highlight command */
          if (!this.networkComponent) return;

          this.networkComponent.highlightNode(command.id);
          break;
        case 'L':
          /* line command */
          if (!this.networkComponent) return;

          this.networkComponent.highlightLine(
              parseInt(command.src, 10), parseInt(command.dst, 10));
          break;
        case 'R':
          /* rank command */
          if (!this.networkComponent) return;

          this.networkComponent.annotateRank(command.id, parseInt(command.rank, 10));
          break;
        case 'N':
          /* neighbor command */
          this.addNode(command.id);
          this.addNode(command.nbrId);
          this.annotateNeighbor(command.id, command.nbrId);
          break;
        case 'P':
          /* packet command */
          if (command.mod === "sent") {
            // packetSent(command.from, command.to, command.seqNum);
          } else {
            // rec
            this.packetReceived(command.from, command.to, command.seqNum, command.latency);
          }
          break;
        case 'DR':
          // annotateDeliveryRatio(command.id, command.from, parseInt(command.rec), parseInt(command.exp));
          break;
        case 'DC':
          /* duty cycle command */
          if (!this.networkComponent) return;

          this.networkComponent.annotateDutyCycle(command.id, parseInt(command.dcOn, 10), parseInt(command.total, 10));
          break;
        default:
          console.log('Unknown command', command.name);
          break;
      }
    }
  }

  addNode(_id) {
    const id = parseInt(_id, 10);
    if (isNaN(id)) {
      return;
    }

    if (this.state.nodes.find( el => { return el.id === `node-${id}`; })) {
      // already added
      return;
    }

    const nodes = _.cloneDeep(this.state.nodes);

    nodes.push({
      id: `node-${id}`,
      group: id === 1 ? 10 : 2
    });

    // also add ipHop, unless we are adding node
    if (id !== 1) {
      const ipHops = _.cloneDeep(this.state.ipHops);
      ipHops.push({
        id: `hop-1-${id}`,
        source: "node-1",
        target: `node-${id}`,
        value: 1
      });

      this.setState({
        nodes: nodes,
        ipHops: ipHops
      });

      return;
    }

    this.setState({ nodes: nodes });
  }

  addLink(_id1, _id2) {
    const id1 = parseInt(_id1, 10);
    const id2 = parseInt(_id2, 10);

    if (isNaN(id1) || isNaN(id2) || id1 === id2) {
      return;
    }

    const from = id1 < id2 ? id1 : id2;
    const to = id1 < id2 ? id2 : id1;
    if (this.state.links.find( el => {
      return el.source === `node-${from}` && el.target === `node-${to}`
    })) {
      // already added
      return;
    }

    const links = _.cloneDeep(this.state.links);

    links.push({
      source: `node-${from}`,
      target: `node-${to}`,
      value: 1
    });

    this.setState({ links: links });
  }

  annotateNeighbor(id, nbrId) {
    this.addLink(id, nbrId);
  }

  // addTableRow(from, to, seqNum, latency) {
  //   let row = table.insertRow(2);
  //   row.insertCell(0).innerHTML = from;
  //   row.insertCell(1).innerHTML = to;
  //   row.insertCell(2).innerHTML = seqNum;
  //   row.insertCell(3).innerHTML = latency;

  //   row.onmouseover = evt => {
  //     row.classList.add("highlight-row");
  //   }

  //   row.onmouseout = evt => {
  //     row.classList.remove("highlight-row");
  //   }

  //   row.onclick = evt => {
  //     highlightNode(from);
  //     highlightNode(to);
  //     highlightIPHop(from, to);
  //   }
  // }

  packetReceived(from, to, seqNum, latency) {
    if (latency > 0) {
      const latencies = this.state.latencies.slice();
      latencies.push(latency);

      const packets = _.cloneDeep(this.state.packets);
      packets.push({
        from: from,
        to: to,
        seqNum: seqNum,
        latency: latency
      });

      this.setState({
        latencies: latencies,
        packets: packets
      });
      
      // let avg = this.latencies.reduce((a, b) => { return a + b }, 0) / this.latencies.length;
      
      if (this.networkComponent) {
        this.networkComponent.highlightIPHop(from, to);
      }

      console.log("Latency:", latency);
    }
    // d3.select("#avg-latency").html(avg);

    // addTableRow(from, to, seqNum, latency);

  }

  // annotateDeliveryRatio(id, from, rec, exp) {
  //   d3.select("node-" + id)
  //       .each(d => {
  //         if (!d.dr) {
  //           d.dr = {};
  //         }
  //         d.dr[from] = {
  //           "rec": rec,
  //           "exp": exp
  //         }
  //       });
  // }

  render() {
    return (
      <Flex align="center" wrap={true}>
        <Box p={1}>
          <Tile title="Network Graph">
            <Network
                ref={comp => { this.networkComponent = comp; }}
                nodes={this.state.nodes}
                links={this.state.links}
                ipHops={this.state.ipHops} />
          </Tile>
        </Box>
        <Box p={1}>
          <Tile title="Latency Histogram">
            <Histogram values={this.state.latencies} />
          </Tile>
        </Box>
        <Box p={1}>
          <Tile title="Packet History">
            <History packets={this.state.packets} />
          </Tile>
        </Box>
      </Flex>
    );
  }
}

ReactDOM.render(
  <div className="app-container">
    <App />
  </div>,
  document.getElementById('root')
);