import React from 'react';
import ReactDOM from 'react-dom';

import './style/PieGroup.css';

import * as d3 from 'd3';

export default class PieGroup extends React.Component {
  constructor(props) {
    super(props);

    this.chartGroup = null;

    this.svgWidth = 900;
    this.svgHeight = 300;

    this.margin = {
      top: 20,
      right: 20,
      bottom: 30,
      left: 20
    }

    this.width = this.svgWidth - this.margin.left - this.margin.right;
    this.height = this.svgHeight - this.margin.top - this.margin.bottom;

    this.piesPerRow = 7;
    this.chartMargin = 15;
    this.chartWidth = this.width / this.piesPerRow;
    this.chartHeight = this.chartWidth;
    this.radius = this.chartWidth / 2 - this.chartMargin;
    this.innerRadius = this.radius - 20;
    
    this.pie = d3.pie().sort(null).value(d => { return d.count; });
    this.arc = d3.arc().outerRadius(this.radius).innerRadius(this.innerRadius);
    this.innerArc = d3.arc()
        .outerRadius(this.innerRadius-2)
        .innerRadius(this.innerRadius-10)
        .startAngle(0)
        .endAngle(2*Math.PI);
  }

  componentDidMount() {
    this.chartGroup = d3.select(ReactDOM.findDOMNode(this.chartGroup));
    this.chartGroup.attr(
        "transform", `translate(${this.margin.left}, ${this.margin.top})`);
  }

  shouldComponentUpdate(nextProps, nextState) {
    const data = nextProps.values;
    // first, handle groups for every pie chart
    const container = this.chartGroup.selectAll(".chart-container").data(data);
    
    container.exit().remove();

    container.enter().append("g")
        .attr("class", "chart-container")
        .attr("id", d => { return `node-${d.id}`; })
        .attr("transform", (d, i) => {
          const x = (i % this.piesPerRow) * this.chartWidth;
          const y = Math.floor(i / this.piesPerRow) * this.chartHeight;
          return `translate(${x+this.chartWidth/2}, ${y+this.chartHeight/2})`;
        });
    
    container
        .attr("id", d => { return `node-${d.id}`; })
        .attr("transform", (d, i) => {
          const x = (i % this.piesPerRow) * this.chartWidth;
          const y = Math.floor(i / this.piesPerRow) * this.chartHeight;
          return `translate(${x+this.chartWidth/2}, ${y+this.chartHeight/2})`;
        });

    // then, draw pie charts
    data.forEach(datum => {
      const group = this.chartGroup.select(`#node-${datum.id}`);

      // labels in the center of pies
      const senderLabels = group.selectAll(".sender-label").data([datum]);
      senderLabels.exit().remove();
      senderLabels.enter().append("text")
          .attr("class", "sender-label")
          .attr("alignment-baseline", "middle")
          .text(d => { return d.id; });
      senderLabels.text(d => { return d.id; });

      const senderCircles = group.selectAll(".sender-circle").data([datum]);
      senderCircles.exit().remove();
      senderCircles.enter().append("path")
          .attr("class", "sender-circle")
          .attr("fill", d => this.props.color(d.id))
          .attr("d", this.innerArc);
      senderCircles.attr("fill", d => { return this.props.color(d.id); });

      // pies
      const path = group.datum(datum.hops).selectAll(".pie-arc").data(this.pie);
      path.exit().remove();
      path.enter().append("path")
          .attr("class", "pie-arc")
          .attr("fill", d => { return this.props.color(d.data.dst); })
          .attr("d", this.arc)
          .on("mouseover", d => {
            d3.select(d3.event.currentTarget)
                .transition(100)
                .attr("transform", "scale(1.1)");
            const l = group.selectAll(".sender-label");
            this.prevLabel = l.text();
            l.text(d.data.dst);
          })
          .on("mouseout", () => {
            d3.select(d3.event.currentTarget)
                .transition(100)
                .attr("transform", "scale(1.0)");
            const l = group.selectAll(".sender-label");
            l.text(this.prevLabel);
          });
      path.attr("fill", d => { return this.props.color(d.data.dst); })
          .attr("d", this.arc);
    });

    return false;
  }

  render() {
    return (
      <div className="pie-group-container">
        <svg width={this.svgWidth} height={this.svgHeight}>
          <g ref={comp => {this.chartGroup = comp;}} />
        </svg>
      </div>
    );
  }
}