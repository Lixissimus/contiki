import React from 'react';

import { Flex, Box } from 'reflexbox';

import StateManager from './StateManager';

import Tile from './Tile';
import Network from './Network';
import Histogram from './Histogram';
import History from './History';
import StripChart from './StripChart';

import './style/Dashboard.css';

export default class Dashboard extends React.Component {
  constructor(props) {
    super(props);

    this.stateManager = new StateManager(this);

    this.bucketSizeElement = null;

    this.networkComponent = null;

    this.state = {
      nodes: [],
      links: [],
      ipHops: [],
      packets: [],
      deliveryRatios: [],
      bucketSize: 50
    }
  }

  componentDidMount() {
    this.bucketSizeElement.value = this.state.bucketSize;
    this.bucketSizeElement.onchange = evt => {
      this.setState({ bucketSize: parseInt(this.bucketSizeElement.value, 10) });
    }    
  }

  annotateRank(id, rank) {
    if (!this.networkComponent) {
      return;
    }

    this.networkComponent.annotateRank(id, rank);
  }

  annotateDutyCycle(id, on, total) {
    if (!this.networkComponent) {
      return;
    }

    this.networkComponent.annotateDutyCycle(id, on, total);
  }

  highlightNode(id) {
    if (!this.networkComponent) {
      return;
    }

    this.networkComponent.highlightNode(id);
  }

  highlightLine(src, dst) {
    if (!this.networkComponent) {
      return;
    }

    this.networkComponent.highlightLine(src, dst);
  }

  highlightIPHop(from, to) {
    if (!this.networkComponent) {
      return;
    }

    this.networkComponent.highlightIPHop(from, to);
  }

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
          <Tile title="Packet History">
            <History packets={this.state.packets} />
          </Tile>
        </Box>
        <Box p={1}>
          <Tile title="Delivery Ratio">
            <StripChart deliveryRatios={this.state.deliveryRatios} />
          </Tile>
        </Box>
        <Box p={1}>
          <Tile title="Latency Histogram">
            <Histogram values={this.state.latencies} bucketSize={this.state.bucketSize} />
            <input type="number" step="5" min="0" ref={comp => { this.bucketSizeElement = comp; }}></input>
          </Tile>
        </Box>
      </Flex>
    );
  }
}