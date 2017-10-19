import React from 'react';
import ReactDOM from 'react-dom';
import * as d3 from 'd3';

import './style/Histogram.css'

export default class Histogram extends React.Component {
  constructor(props) {
    super(props);

    this.svgWidth = 960;
    this.svgHeight = 300;

    this.margin = {
      top: 20,
      right: 20,
      bottom: 60,
      left: 40
    }

    this.bucketArray = [];
    this.dataArray = [];

    this.bSize = 50;
    this.maxYTicks = 5;

    this.width = this.svgWidth - this.margin.left - this.margin.right;
    this.height = this.svgHeight - this.margin.top - this.margin.bottom;

    this.x = d3.scaleBand().rangeRound([0, this.width]).padding(0.1);
    this.y = d3.scaleLinear().rangeRound([this.height, 0]);
  }

  componentDidMount() {
    this.d3Histogram = d3.select(ReactDOM.findDOMNode(this.refs.histogram));
    this.d3Histogram.attr(
        "transform", `translate(${this.margin.left}, ${this.margin.top})`);
    
    this.xAxis = this.d3Histogram.append("g")
        .attr("class", "axis axis--x")
        .attr("transform", `translate(0, ${this.height})`);
    this.xAxis.call(d3.axisBottom(this.x));
    
    this.yAxis = this.d3Histogram.append("g")
        .attr("class", "axis axis--y");
    this.yAxis.call(
        d3.axisLeft(this.y)
          .ticks(1)
          .tickFormat(d3.format("d")));
  }

  shouldComponentUpdate(nextProps, nextState) {
    // Todo: handle bucketSize change

    this.bucketArray = this.groupData(nextProps.values);

    this.d3Histogram = d3.select(ReactDOM.findDOMNode(this.refs.histogram));

    // update scale domains
    this.x.domain(this.bucketArray.map(d => { return d.name }));
    this.y.domain([0, d3.max(this.bucketArray, d => { return d.value; })]);

    const xScale = d3.axisBottom().scale(this.x);
    const yScale = d3.axisLeft().scale(this.y)
        .ticks(Math.min(
            d3.max(this.bucketArray, d => { return d.value; }), this.maxYTicks))
        .tickFormat(d3.format("d"));
    
    this.xAxis.transition().duration(300)
        .call(xScale)
        .selectAll("text")
          .attr("x", 9)
          .attr("y", 0)
          .attr("transform", "rotate(90)")
          .attr("dy", ".35em")
          .style("text-anchor", "start");

    this.yAxis.transition().duration(300).call(yScale);
    
    const bars = this.d3Histogram.selectAll(".bar").data(this.bucketArray);

    // remove unneeded bars
    bars.exit().remove();

    // draw new bars
    bars.enter().append("rect")
        .attr("class", "bar")
        .attr("x", d => { return this.x(d.name); })
        .attr("y", d => { return this.y(d.value); })
        .attr("width", this.x.bandwidth())
        .attr("height", d => { return this.height - this.y(d.value) });

    // update existing bars
    bars.transition().duration(500)
        .attr("x", d => { return this.x(d.name); })
        .attr("y", d => { return this.y(d.value); })
        .attr("width", this.x.bandwidth())
        .attr("height", d => { return this.height - this.y(d.value); });

    return false;
  }

  groupData(data) {
    const buckets = {};

    // create and fill buckets
    let firstBucket = Number.MAX_SAFE_INTEGER;
    let lastBucket = 0;
    data.forEach(d => {
      const bucket = Math.floor(d / this.props.bucketSize);
      if (bucket > lastBucket) {
        lastBucket = bucket;
      }
      if (bucket < firstBucket) {
        firstBucket = bucket;
      }
      if (!buckets[bucket]) {
        buckets[bucket] = 1;
      } else {
        buckets[bucket]++;
      }
    });
    
    // add empty buckets
    for (let i = firstBucket; i <= lastBucket; i++) {
      if (!buckets[i]) {
        buckets[i] = 0;
      }
    }

    // convert buckets to array
    const ret = [];
    for (let bucket in buckets) {
      ret.push({
        name: `${bucket*this.props.bucketSize}-${bucket*this.props.bucketSize + this.props.bucketSize}`,
        bucket: bucket,
        value: buckets[bucket]
      });
    }

    // sort bucket array by bucket index
    ret.sort((a, b) => {
      return a.bucket - b.bucket;
    });

    return ret;
  }

  render() {
    return (
      <svg width={this.svgWidth} height={this.svgHeight}>
        <g ref="histogram" />
      </svg>
    );
  }
}

Histogram.defaultProps = { bucketSize: 50 };
