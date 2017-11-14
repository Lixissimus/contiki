import React from 'react';

import * as d3 from 'd3';

import { Flex, Box } from 'reflexbox';

import StateManager from './StateManager';
import EventQueue from './EventQueue';

import Tile from './Tile';
import Network from './Network';
import Histogram from './Histogram';
import History from './History';
import StripChart from './StripChart';
import HistoryChart from './HistoryChart';
import Persistence from './Persistence';
import PieGroup from './PieGroup';

import './style/Dashboard.css';

export default class Dashboard extends React.Component {
  constructor(props) {
    super(props);

    this.eventQueue = new EventQueue();
    this.stateManager = new StateManager(this);

    this.eventQueue.subscribe("set-time", data => {
      this.stateManager.setTime(data.time);
    });

    this.eventQueue.subscribe("set-history", data => {
      this.stateManager.setHistory(data.history);
    });

    this.eventQueue.subscribe("connect-ws", data => {
      this.stateManager.setupCommunication(data.url);
    });
    
    this.eventQueue.subscribe("connect-remote", data => {
      this.stateManager.connectRemote(data.url);
    });

    this.bucketSizeElement = null;

    this.color = d3.scaleOrdinal(d3.schemeCategory20);

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

    d3.select("body").on("keydown", () => {
      if (d3.event.keyCode === 27) {
        // esc
        this.eventQueue.post("esc-pressed", {});
      }
  });
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
          <Tile title="Next Hops">
            <PieGroup 
                values={this.state.nextHops}
                color={this.color}
                post={this.eventQueue.post.bind(this.eventQueue)}
                subscribe={this.eventQueue.subscribe.bind(this.eventQueue)} />
          </Tile>
        </Box>
        <Box p={1}>
          <Tile title="Network Graph">
            <Network
                nodes={this.state.nodes}
                links={this.state.links}
                ipHops={this.state.ipHops}
                color={this.color}
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
                color={this.color}
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
          <Tile title="Persistence">
            <Persistence
                getData={() => { return this.stateManager.history; }}
                post={this.eventQueue.post.bind(this.eventQueue)}
                subscribe={this.eventQueue.subscribe.bind(this.eventQueue)} />
          </Tile>
        </Box>
        <Box p={1}>
          <Tile title="Total Delivery Ratio">
            <HistoryChart
                values={this.state.avgRatios}
                post={this.eventQueue.post.bind(this.eventQueue)}
                subscribe={this.eventQueue.subscribe.bind(this.eventQueue)} />
          </Tile>
        </Box>
        <Box p={1}>
          <Tile title="Average Duty Cycle">
            <HistoryChart
                values={this.state.avgDutyCycles}
                post={this.eventQueue.post.bind(this.eventQueue)}
                subscribe={this.eventQueue.subscribe.bind(this.eventQueue)} />
          </Tile>
        </Box>
        <Box p={1}>
          <Tile title="Average Latency">
            <HistoryChart
                values={this.state.avgLatencies}
                post={this.eventQueue.post.bind(this.eventQueue)}
                subscribe={this.eventQueue.subscribe.bind(this.eventQueue)} />
          </Tile>
        </Box>
      </Flex>
    );
  }
}