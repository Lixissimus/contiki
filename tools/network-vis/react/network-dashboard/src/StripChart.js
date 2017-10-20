import React from 'react';
import ReactDOM from 'react-dom';

import * as d3 from 'd3';

import './style/StripChart.css';

export default class StripChart extends React.Component {
  constructor(props) {
    super(props);

    this.svgWidth = 450;
    this.svgHeight = 300;

    this.margin = {
      top: 20,
      right: 20,
      bottom: 30,
      left: 40
    }

    this.width = this.svgWidth - this.margin.left - this.margin.right;
    this.height = this.svgHeight - this.margin.top - this.margin.bottom;

    this.x = d3.scaleLinear().rangeRound([0, this.width]).domain([0, 1]);
    this.y = d3.scaleBand().rangeRound([0, this.height]).padding(0.1);
  }

  componentDidMount() {
    this.stripChart = d3.select(ReactDOM.findDOMNode(this.refs.stripChart));
    this.stripChart.attr(
        "transform", `translate(${this.margin.left}, ${this.margin.top})`);
    this.barsReceived = this.stripChart.append("g")
        .attr("class", "strips-received");
    this.barsMissed = this.stripChart.append("g")
        .attr("class", "strips-missed");
    this.labels = this.stripChart.append("g")
        .attr("class", "labels");
  }

  shouldComponentUpdate(nextProps, nextState) {
    const data = nextProps.deliveryRatios;
    if (data.length < 1) {
      return false;
    }

    // update domain of y scale to fit all strips
    this.y.domain(data.map(el => { return el.id }))


    const barsReceived = this.barsReceived.selectAll("rect").data(data);
    
    barsReceived.exit().remove();

    barsReceived.enter().append("rect")
        .attr("class", "strip")
        .attr("x", 0)
        .attr("y", d => { return this.y(d.id); })
        .attr("width", d => { return this.x(d.received / (d.sent)); })
        .attr("height", this.y.bandwidth());
        
    barsReceived.transition(500)
        .attr("y", d => { return this.y(d.id); })
        .attr("width", d => { return this.x(d.received / (d.sent)); })
        .attr("height", this.y.bandwidth());
    
    const barsMissed = this.barsMissed.selectAll("rect").data(data);
    
    barsMissed.exit().remove();

    barsMissed.enter().append("rect")
        .attr("class", "strip")
        .attr("x", d => { return this.x(d.received / d.sent); })
        .attr("y", d => { return this.y(d.id); })
        .attr("width", d => { return this.x(1 - (d.received / d.sent)); })
        .attr("height", this.y.bandwidth());
    
    barsMissed.transition(500)
        .attr("x", d => { return this.x(d.received / d.sent); })
        .attr("y", d => { return this.y(d.id); })
        .attr("width", d => { return this.x(1 - (d.received / d.sent)); })
        .attr("height", this.y.bandwidth());

    const labels = this.labels.selectAll("text").data(data);

    labels.exit().remove();

    labels.enter().append("text")
        .attr("alignment-baseline", "middle")
        .attr("x", 5)
        .attr("y", d => { return this.y(d.id) + this.y.bandwidth() / 2; })
        .text(d => { return `Node-${d.id}: ${d.received}/${d.sent}`; });

    labels.transition(500)
        .attr("y", d => { return this.y(d.id) + this.y.bandwidth() / 2; })
        .text(d => { return `Node-${d.id}: ${d.received}/${d.sent}`; });

    return false;
  }

  render() {
    return (
      <svg width={this.svgWidth} height={this.svgHeight}>
        <g ref="stripChart" />
      </svg>
    );
  }
}