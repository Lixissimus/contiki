import React from 'react';

import { Flex, Box } from 'reflexbox';

import StateManager from './StateManager';
import EventQueue from './EventQueue';

import Tile from './Tile';
import Network from './Network';
import Histogram from './Histogram';
import History from './History';
import StripChart from './StripChart';
import HistoryChart from './HistoryChart';

import './style/Dashboard.css';

export default class Dashboard extends React.Component {
  constructor(props) {
    super(props);

    this.eventQueue = new EventQueue();
    this.stateManager = new StateManager(this);

    this.eventQueue.subscribe("set-time", data => {
      this.stateManager.setTime(data.time);
    });

    this.bucketSizeElement = null;

    this.timeComponent = null;

    this.state = {
      bucketSize: 50,
      nodes: [],
      links: [],
      ipHops: [],
      packets: [],
      deliveryRatios: [],
      avgRatios: []
    }
  }

  componentDidMount() {
    this.bucketSizeElement.value = this.state.bucketSize;
    this.bucketSizeElement.onchange = evt => {
      this.setState({ bucketSize: parseInt(this.bucketSizeElement.value, 10) });
    }
  }

  annotateRank(id, rank) {
    this.eventQueue.post("annotate-rank", { id: id, rank: rank });
  }

  annotateDutyCycle(id, on, total) {
    this.eventQueue.post("annotate-dc", { id: id, on: on, total: total });
  }

  highlightNode(id) {
    this.eventQueue.post("highlight-node", { id: id });
  }

  highlightLine(src, dst) {
    this.eventQueue.post("highlight-link", { src: src, dst: dst });
  }

  highlightIPHop(src, dst) {
    this.eventQueue.post("highlight-ipHop", { src: src, dst: dst });
  }

  render() {
    return (
      <Flex align="center" wrap={true}>
        <Box p={1}>
          <Tile title="Network Graph">
            <Network
                nodes={this.state.nodes}
                links={this.state.links}
                ipHops={this.state.ipHops}
                post={this.eventQueue.post.bind(this.eventQueue)}
                subscribe={this.eventQueue.subscribe.bind(this.eventQueue)} />
          </Tile>
        </Box>
        <Box p={1}>
          <Tile title="Packet History">
            <History 
                packets={this.state.packets}
                post={this.eventQueue.post.bind(this.eventQueue)}
                subscribe={this.eventQueue.subscribe.bind(this.eventQueue)} />
          </Tile>
        </Box>
        <Box p={1}>
          <Tile title="Delivery Ratio">
            <StripChart
                deliveryRatios={this.state.deliveryRatios}
                post={this.eventQueue.post.bind(this.eventQueue)}
                subscribe={this.eventQueue.subscribe.bind(this.eventQueue)} />
          </Tile>
        </Box>
        <Box p={1}>
          <Tile title="Latency Histogram">
            <Histogram 
                values={this.state.latencies}
                bucketSize={this.state.bucketSize}
                post={this.eventQueue.post.bind(this.eventQueue)}
                subscribe={this.eventQueue.subscribe.bind(this.eventQueue)} />
            <input type="number" step="5" min="0" ref={comp => { this.bucketSizeElement = comp; }}></input>
          </Tile>
        </Box>
        <Box p={1}>
          <Tile title="Total Delivery Ratio">
            <HistoryChart
                ref={comp => { this.timeComponent = comp; }}
                values={this.state.avgRatios}
                post={this.eventQueue.post.bind(this.eventQueue)}
                subscribe={this.eventQueue.subscribe.bind(this.eventQueue)} />
          </Tile>
        </Box>
      </Flex>
    );
  }
}