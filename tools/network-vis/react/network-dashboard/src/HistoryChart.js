import React from 'react';
import ReactDOM from 'react-dom';

import * as d3 from 'd3';

import './style/HistoryChart.css';

export default class HistoryChart extends React.Component {
  constructor(props) {
    super(props);

    this.svgWidth = 960;
    this.svgHeight = 300;

    this.margin = {
      top: 20,
      right: 20,
      bottom: 30,
      left: 40
    }

    this.data = [];

    this.selectedDate = null;
    this.locked = false;
    this.chart = null;

    this.width = this.svgWidth - this.margin.left - this.margin.right;
    this.height = this.svgHeight - this.margin.top - this.margin.bottom;

    this.x = d3.scaleLinear().rangeRound([0, this.width]);
    this.y = d3.scaleLinear().rangeRound([this.height, 0]);
    this.y.domain([0, 1]);
  }

  componentDidMount() {
    const _this = this;
    d3.select("body").on("keydown", () => {
        if (d3.event.keyCode === 27) {
          // esc
          _this.locked = false;
          _this.props.post("set-time", { time: -1 });
          _this.focus.style("display", "none");
          _this.lock.style("display", "none");
        }
    });

    this.chart = d3.select(ReactDOM.findDOMNode(this.chart));
    this.chart.attr(
        "transform", `translate(${this.margin.left}, ${this.margin.top})`);

    this.xAxis = this.chart.append("g")
        .attr("class", "axis axis--x history-chart")
        .attr("transform", `translate(0, ${this.height})`);
    this.xAxis.call(
        d3.axisBottom(this.x)
          .ticks(10)
          .tickFormat(d3.format("d")));
    
    this.yAxis = this.chart.append("g")
        .attr("class", "axis axis--y history-chart");
    this.yAxis.call(
        d3.axisLeft(this.y)
          .ticks(5));
          // .tickFormat(d3.format("d")));
    
    this.graph = this.chart.append("g")
        .attr("class", "graph");

    this.line = d3.line()
        .x(d => { return this.x(d.timestamp); })
        .y(d => { return this.y(d.ratio); })
    this.graph.append("path")
        .attr("class", "line history-chart");

    this.lock = this.chart.append("g").style("display", "none");

    this.lock.append("line")
        .attr("id", "lockLineX")
        .attr("class", "history-chart lock-line");
    this.lock.append("circle")
        .attr("id", "lockCircle")
        .attr("class", "history-chart lock-circle")
        .attr("r", 5);

    this.focus = this.chart.append("g").style("display", "none");

    this.focus.append("line")
        .attr("id", "focusLineX")
        .attr("class", "history-chart focus-line");
    this.focus.append("line")
        .attr("id", "focusLineY")
        .attr("class", "history-chart focus-line");
    this.focus.append("circle")
        .attr("id", "focusCircle")
        .attr("class", "history-chart focus-circle")
        .attr("r", 5);

    this.overlay = this.chart.append("rect")
        .attr("class", "history-chart overlay")
        .attr("width", this.width)
        .attr("height", this.height)
        .on("mouseout", () => {
          if (_this.locked) {
            return;
          }
          this.focus.style("display", "none");
        })
        .on("mousemove", () => {
          this.focus.style("display", null);
          if (!_this.data.length) {
            return;
          }

          const mouse = d3.mouse(d3.event.currentTarget);
          const mouseDate = _this.x.invert(mouse[0]);

          const dataIdx = this.closestDateIdx(mouseDate);
          if (this.data[dataIdx] === _this.selectedDate) {
            // nothing changed
            return;
          }
          _this.selectedDate = this.data[dataIdx];
          
          _this.focus.select("#focusLineX")
              .attr("x1", _this.x(_this.selectedDate.timestamp))
              .attr("y1", _this.y(0))
              .attr("x2", _this.x(_this.selectedDate.timestamp))
              .attr("y2", _this.y(1))
          _this.focus.select("#focusLineY")
              .attr("x1", _this.x(0))
              .attr("y1", _this.y(_this.selectedDate.ratio))
              .attr("x2", _this.width)
              .attr("y2", _this.y(_this.selectedDate.ratio))
          _this.focus.select("#focusCircle")
              .attr("cx", _this.x(_this.selectedDate.timestamp))
              .attr("cy", _this.y(_this.selectedDate.ratio));
        })
        .on("click", () => {
          _this.locked = true;

          _this.lock.style("display", null);
          _this.lock.select("#lockLineX")
              .attr("x1", _this.x(_this.selectedDate.timestamp))
              .attr("y1", _this.y(0))
              .attr("x2", _this.x(_this.selectedDate.timestamp))
              .attr("y2", _this.y(1))
          _this.lock.select("#lockCircle")
              .attr("cx", _this.x(_this.selectedDate.timestamp))
              .attr("cy", _this.y(_this.selectedDate.ratio));

          _this.props.post(
              "set-time", { time: _this.selectedDate.timeIndex });
        });
  }

  shouldComponentUpdate(nextProps, nextState) {
    if (nextProps.values.length < this.data.length) {
      // do not accept values from the past
      return false;
    }
    this.data = nextProps.values;
    if (this.data.length < 1) {
      return false;
    }
    // update scale domain
    this.x.domain([0, this.data[this.data.length-1].timestamp]);
    const xScale = d3.axisBottom().scale(this.x);
    this.xAxis.transition().duration(300).call(xScale);

    const dots = this.graph.selectAll("circle").data(this.data);
    
    dots.exit().remove();

    dots.enter().append("circle")
        .attr("class", "history-chart")
        .attr("r", 2)
        .attr("cx", d => { return this.x(d.timestamp); })
        .attr("cy", d => { return this.y(d.ratio); })

    dots.transition(500)
        .attr("cx", d => { return this.x(d.timestamp); });

    this.graph.selectAll(".line").transition(500).attr("d", this.line(this.data));

    return false;
  }

  closestDateIdx(time) {
    let i = 0;
    let j = this.data.length-1;
    while (j > i+1) {
      let c = Math.floor((i+j)/2);
      if (this.data[c].timestamp > time) {
        j = c;
      } else {
        i = c;
      }
    }

    return time - this.data[i].timestamp < this.data[j].timestamp - time ? i : j;
  }

  render() {
    return (
      <svg width={this.svgWidth} height={this.svgHeight}>
        <g ref={comp => { this.chart = comp }} />
      </svg>
    );
  }
}
