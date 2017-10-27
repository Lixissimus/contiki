import React from 'react';
import ReactDOM from 'react-dom';
import * as d3 from 'd3';

import * as _ from 'lodash';

import './style/Network.css'

export default class Network extends React.Component {
  constructor(props) {
    super(props);

    this.data = [];

    this.svgWidth = 600;
    this.svgHeight = 300;

    this.margin = {
      top: 20,
      right: 20,
      bottom: 30,
      left: 40
    }

    this.width = this.svgWidth - this.margin.left - this.margin.right;
    this.height = this.svgHeight - this.margin.top - this.margin.bottom;

    this.radius = 10;

    this.simulation = d3.forceSimulation()
        .force("link", d3.forceLink().id(d => { return d.id; }))
        .force("charge", d3.forceManyBody().strength(d => { return -200; } ))
        .force("center", d3.forceCenter(this.svgWidth / 2, this.svgHeight / 2));
    this.simulation.on("tick", this.ticked.bind(this));

    this.props.subscribe("highlight-node", data => {
      this.highlightNode(data.id);
    });
    this.props.subscribe("highlight-link", data => {
      this.highlightLine(data.src, data.dst);
    });
    this.props.subscribe("highlight-ipHop", data => {
      this.highlightIPHop(data.src, data.dst);
    });
    this.props.subscribe("annotate-rank", data => {
      this.annotateRank(data.id, data.rank);
    });
    this.props.subscribe("annotate-dc", data => {
      this.annotateDutyCycle(data.id, data.on, data.total);
    });

  }

  componentDidMount() {
    this.container = d3.select(ReactDOM.findDOMNode(this.refs.container));
    this.d3Network = d3.select(ReactDOM.findDOMNode(this.refs.network));
    
    this.ipHops = this.d3Network.append("g", ":first-child")
        .attr("class", "ipHops");
    this.links = this.d3Network.append("g").attr("class", "links");
    this.nodes = this.d3Network.append("g").attr("class", "nodes");
    this.labels = this.d3Network.append("g").attr("class", "labels");

    this.tooltip = this.container.append("div")
        .attr("class", "tooltip")
        .style("opacity", 0);
  }

  shouldComponentUpdate(nextProps, nextState) {
    // copy data
    // somehow things break when copying nodes, too
    // also things break when not copying links,
    // not sure why...
    const data = {
      nodes: nextProps.nodes,
      links: _.cloneDeep(nextProps.links),
      ipHops: _.cloneDeep(nextProps.ipHops)
    }

    // check which new elements have been added
    const nodesChanged = data.nodes.length !== this.props.nodes.length;
    const linksChanged = data.links.length !== this.props.links.length;
    const ipHopsChanged = data.ipHops.length !== this.props.ipHops.length;


    if (linksChanged) {
      const links = this.links.selectAll("line").data(data.links);
      links.exit().remove();
      links.enter().append("line")
          .attr("id", d => { return `${d.source}-${d.target}`; })
          .attr("stroke-width", d => { return d.value; });
    }

    if (nodesChanged) {
      const nodes = this.nodes.selectAll("circle").data(data.nodes);
      nodes.exit().remove();
      nodes.enter().append("circle")
          .attr("id", d => { return d.id; })
          .attr("r", this.radius)
          .attr("fill", d => { return this.props.color(d.id.split("-")[1]); })
          .call(d3.drag()
              .on("start", this.dragStarted.bind(this))
              .on("drag", this.dragged.bind(this))
              .on("end", this.dragEnded.bind(this)))
          .on("mouseover", d => {
              this.tooltip.transition()
                  .duration(200)
                  .style("opacity", .9);
              this.tooltip.html(`${d.id} <br> rank: ${d.rank} <br> dc: ${d.dc}%`)
                  .style("left", (d3.event.pageX + 10) + "px")
                  .style("top", (d3.event.pageY - 28) + "px");
          })
          .on("mouseout", d => {		
              this.tooltip.transition()
                .duration(500)
                .style("opacity", 0);
          });
      nodes
          .attr("id", d => { return d.id; })
          .attr("fill", d => { return this.props.color(d.id.split("-")[1]); });

      const labels = this.labels.selectAll("text").data(data.nodes);
      labels.exit().remove();
      labels.enter().append("text")
          .attr("class", "node-label")
          .attr("alignment-baseline", "middle")
          .attr("pointer-events", "none")
          .attr("id", d => { return `label-node-${d.id.split("-")[1]}`; })
          .text(d => { return d.id.split("-")[1]; });
      labels
          .attr("id", d => { return `label-node-${d.id.split("-")[1]}`; })
          .text(d => { return d.id.split("-")[1]; });
    }
    
    // if (ipHopsChanged) {
      // draw ipHops, but only if we have drawn root already
      if (!this.nodes.select("#node-1").empty()) {
        let ipHops = this.ipHops.selectAll("line").data(data.ipHops);
        ipHops.exit().remove();
        ipHops.enter().append("line")
              .attr("id", d => { return d.id; })
              .attr("x1", d => { return this.nodes.select("#" + d.source).attr("cx"); })
              .attr("y1", d => { return this.nodes.select("#" + d.source).attr("cy"); })
              .attr("x2", d => { return this.nodes.select("#" + d.target).attr("cx"); })
              .attr("y2", d => { return this.nodes.select("#" + d.target).attr("cy"); })
              .style("opacity", 0);
      }
    // }

    // if anything changed, update and re-charge simulation
    if (linksChanged || nodesChanged || ipHopsChanged) {
      this.simulation.nodes(data.nodes);
      this.simulation.force("link").links(data.links);

      this.simulation.restart();
      this.simulation.alpha(0.3);
    }
    
    return false;
  }

  dragStarted(d) {
    if (!d3.event.active) {
      this.simulation.alphaTarget(0.3).restart();
    }
    d.fx = d.x;
    d.fy = d.y;
  }

  dragged(d) {
    d.fx = d3.event.x;
    d.fy = d3.event.y;
  }

  dragEnded(d) {
    if (!d3.event.active) {
      this.simulation.alphaTarget(0);
    }
    d.fx = null;
    d.fy = null;
  }

  ticked() {
    this.links.selectAll("line")
        .attr("x1", d => { return this.nodes.select("#" + d.source.id).attr("cx"); })
        .attr("y1", d => { return this.nodes.select("#" + d.source.id).attr("cy"); })
        .attr("x2", d => { return this.nodes.select("#" + d.target.id).attr("cx"); })
        .attr("y2", d => { return this.nodes.select("#" + d.target.id).attr("cy"); });

    this.ipHops.selectAll("line")
        .attr("x1", d => { return this.nodes.select("#" + d.source).attr("cx"); })
        .attr("y1", d => { return this.nodes.select("#" + d.source).attr("cy"); })
        .attr("x2", d => { return this.nodes.select("#" + d.target).attr("cx"); })
        .attr("y2", d => { return this.nodes.select("#" + d.target).attr("cy"); });
    this.nodes.selectAll("circle")
        .attr("cx", d => { return d.x; })
        .attr("cy", d => { return d.y; });
    this.labels.selectAll("text")
        .attr("x", d => { return d.x; })
        .attr("y", d => { return d.y+1; });
  }

  highlightNode(id) {
    this.nodes.select("#node-" + id)
        .transition()
          .duration(200)
          .attr("r", this.radius + 5)
        .transition()
          .delay(600)
          .duration(500)
          .attr("r", this.radius);
  }

  highlightLine(src, dst) {
    let from = src < dst ? src : dst;
    let to = src < dst ? dst : src;
    this.links.select(`#node-${from}-node-${to}`)
        .transition()
          .duration(200)
          .attr("stroke-width", 3)
        .transition()
          .delay(600)
          .duration(500)
          .attr("stroke-width", 1);
  }

  highlightIPHop(src, dst) {
    this.ipHops.select(`#hop-${dst}-${src}`)
        .transition()
          .delay(500)
          .duration(500)
          .style("opacity", .6)
        .transition()
          .delay(1000)
          .duration(200)
          .style("opacity", 0);
  }

  annotateRank(id, rank) {
    this.nodes.select("#node-" + id)
        .each(d => {
          d.rank = rank;
        });
  }

  annotateDutyCycle(id, dcOn, total) {
    this.nodes.select("#node-" + id)
        .each(d => {
          d.dc = Math.round(dcOn / total * 10000) / 100;
        });
  }

  render() {
    return (
      <div ref="container">
        <svg width={this.svgWidth} height={this.svgHeight}>
          <g ref="network" />
        </svg>
      </div>
    );
  }
}
