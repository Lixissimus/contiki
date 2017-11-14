import * as _ from 'lodash';

export default class StateManager {
  constructor(dashboard) {
    this.dashboard = dashboard;

    this.history = [];
    this.deliveryRatios = {};
    this.nextHops = {};
    this.dutyCycles = {};
    this.isLive = true;
    this.time = -1;

    this.connection = null;

    this.state = {
      bucketSize: 50,
      latencies: [],
      avgLatencies: [],
      nodes: [],
      links: [],
      ipHops: [],
      packets: [],
      deliveryRatios: [],
      avgRatios: [],
      nextHops: [],
      dutyCycles: [],
      avgDutyCycles: []
    };
  }
  
  addState(diff) {
    const newState = Object.assign({}, this.state, diff);
    this.history.push(newState);
    this.state = newState;
    
    if (this.isLive) {
      this.dashboard.setState(newState);
    }
  }

  setupCommunication(ip) {
    // const connectButton = d3.select("#connect-button");
    // const ipField = d3.select("#ip-field");

    // connectButton.on("click", () => {
    //   const ip = ipField.property("value");
    //   connection.send(JSON.stringify({
    //     type: "connect",
    //     ip: ip
    //   }));
    // });

    this.connection = new WebSocket(`ws://${ip}`);

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
          if (this.isLive) {
            this.dashboard.highlightNode(command.id);
          }
          break;
        case 'L':
          /* line command */
          this.linkLayerHop(
              parseInt(command.src, 10), parseInt(command.dst, 10));
          break;
        case 'R':
          /* rank command */
          if (this.isLive) {
            this.dashboard.annotateRank(command.id, parseInt(command.rank, 10));
          }
          break;
        case 'N':
          /* neighbor command */
          this.addNode(command.id);
          this.addNode(command.nbrId);
          this.addLink(command.id, command.nbrId);
          break;
        case 'P':
          /* packet command */
          if (command.mod === "sent") {
            this.packetSent(command.from, command.to, command.seqNum);
          } else {
            // rec
            this.packetReceived(command.from, command.to, command.seqNum, command.latency);
          }
          break;
        case 'DR':
          // not used anymore
          // annotateDeliveryRatio(command.id, command.from, parseInt(command.rec), parseInt(command.exp));
          break;
        case 'DC':
          /* duty cycle command */
          this.dutyCycle(command.id, parseInt(command.dcOn, 10), parseInt(command.total, 10));
          if (this.isLive) {
            this.dashboard.annotateDutyCycle(command.id, parseInt(command.dcOn, 10), parseInt(command.total, 10));
          }
          break;
        default:
          console.log('Unknown command', command.name);
          break;
      }
    }
  }

  connectRemote(ip) {
    this.connection.send(JSON.stringify({
      type: "connect",
      ip: ip
    }));
  }

  setTime(time) {
    if (time >= this.history.length) {
      // out of range
      return;
    }

    if (time >= 0){
      this.isLive = false;
      // this.addState(this.history[time]);
      this.dashboard.setState(this.history[time]);
    } else {
      this.isLive = true;
      this.dashboard.setState(this.history[this.history.length-1]);
    }
  }

  setHistory(history) {
    this.history = history;
    this.dashboard.setState(history[history.length-1]);
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

    // also add ipHop, unless we are adding root node
    if (id !== 1) {
      const ipHops = _.cloneDeep(this.state.ipHops);
      ipHops.push({
        id: `hop-1-${id}`,
        source: "node-1",
        target: `node-${id}`,
        value: 1
      });

      this.addState({
        nodes: nodes,
        ipHops: ipHops
      });

      return;
    }

    this.addState({ nodes: nodes });
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

    this.addState({ links: links });
  }

  linkLayerHop(src, dst) {
    if (!this.nextHops[src]) {
      this.nextHops[src] = {};
    }
    if (!this.nextHops[src][dst]) {
      this.nextHops[src][dst] = 1;
    } else {
      this.nextHops[src][dst]++;
    }

    const nextHops = Object.keys(this.nextHops).map(key => {
      return {
        id: key,
        // is copying necessary here?
        // hops: Object.assign({}, this.nextHops[key])
        // hops: this.nextHops[key]
        hops: Object.keys(this.nextHops[key]).map(dst => {
          return {
            dst: dst,
            count: this.nextHops[key][dst]
          }
        })
      }
    });

    this.addState({
      nextHops: nextHops
    });
    
    if (this.isLive) {
      this.dashboard.highlightLine(src, dst);
    }
  }
  
  dutyCycle(id, on, total) {
    // if (!this.dutyCycles[id]) {
      this.dutyCycles[id] = on / total;
    // }

    const dutyCycles = _.cloneDeep(this.state.dutyCycles);
    dutyCycles.push(Object.assign({}, this.dutyCycles));

    const keys = Object.keys(this.dutyCycles)
    const avg = keys.length ? 
    keys.reduce((prev, key) => {
      return prev += this.dutyCycles[key];
    }, 0) / keys.length : 0;
    
    const avgDutyCycles = _.cloneDeep(this.state.avgDutyCycles);
    avgDutyCycles.push({
      timestamp: window.performance.now() / 1000,
      timeIndex: this.history.length,
      value: avg
    });

    this.addState({
      dutyCycles: dutyCycles,
      avgDutyCycles: avgDutyCycles
    });
  }

  packetSent(from, to, _seqNum) {
    if (!this.deliveryRatios[from]) {
      this.deliveryRatios[from] = {
        sent: 1,
        received: 0,
        lastSeqNum: -1
      };
    } else {
      this.deliveryRatios[from].sent++;
    }
  }

  packetReceived(from, to, _seqNum, latency) {
    if (latency < 0) {
      return;
    }
    
    const seqNum = parseInt(_seqNum, 10);

    const latencies = this.state.latencies.slice();
    latencies.push(latency);

    const avgLatencies = _.cloneDeep(this.state.avgLatencies);
    const avgLatency = latencies.reduce((prev, lat) => {
      return prev + lat;
    }, 0) / latencies.length;

    avgLatencies.push({
      timestamp: window.performance.now() / 1000,
      timeIndex: this.history.length,
      value: avgLatency
    });

    const packets = _.cloneDeep(this.state.packets);
    packets.push({
      from: from,
      to: to,
      seqNum: seqNum,
      latency: latency
    });

    // const deliveryRatios = _.cloneDeep(this.state.deliveryRatios);
    // do not count duplicates
    if (this.deliveryRatios[from] && this.deliveryRatios[from].lastSeqNum < seqNum) {
      this.deliveryRatios[from].received++;
      this.deliveryRatios[from].lastSeqNum = seqNum;
    }

    const deliveryRatios = Object.keys(this.deliveryRatios).length ?
        Object.keys(this.deliveryRatios).map(key => {
          return Object.assign({ id: key }, this.deliveryRatios[key]);
        }) : [];

    const avgRatio = Object.keys(this.deliveryRatios).reduce((prev, key) => {
      return prev + this.deliveryRatios[key].received / this.deliveryRatios[key].sent;
    }, 0) / Object.keys(this.deliveryRatios).length;

    const avgRatios = _.cloneDeep(this.state.avgRatios);
    avgRatios.push({
      timestamp: window.performance.now() / 1000,
      timeIndex: this.history.length,
      value: avgRatio
    });

    this.addState({
      latencies: latencies,
      avgLatencies: avgLatencies,
      packets: packets,
      deliveryRatios: deliveryRatios,
      avgRatios: avgRatios
    });
    
    if (this.isLive) {
      this.dashboard.highlightIPHop(from, to);
    }
  }
}
