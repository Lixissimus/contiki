import React from 'react';
import ReactDOM from 'react-dom';
import * as d3 from 'd3';

import './style/History.css'

export default class History extends React.Component {
  constructor(props) {
    super(props);

    this.filterBy = null;
  }
  componentDidMount() {
    this.tableHead = d3.select(ReactDOM.findDOMNode(this.refs.tableHead));
    this.tableBody = d3.select(ReactDOM.findDOMNode(this.refs.tableBody));
    this.tableContainer = this.refs.tableContainer;

    const _this = this;
    this.props.subscribe("node-select", data => {
      _this.filterBy = data.id;
    });
    this.props.subscribe("esc-pressed", data => {
      _this.filterBy = null;
    });
  }

  shouldComponentUpdate(nextProps, nextState) {
    const data = this.filter(nextProps.packets);
    const rows = this.tableBody.selectAll("tr").data(data);
    const isScrolledDown = this.tableContainer.scrollTop === this.tableContainer.scrollHeight - this.tableContainer.clientHeight;

    rows.exit().remove();
    rows.enter().append("tr")
        .html(d => {
          return (`
              <td>${d.from}</td>
              <td>${d.to}</td>
              <td>${d.seqNum}</td>
              <td>${Math.round(d.latency*100) / 100}</td>`
          );
        });
    rows.html(d => {
      return (`
          <td>${d.from}</td>
          <td>${d.to}</td>
          <td>${d.seqNum}</td>
          <td>${Math.round(d.latency*100) / 100}</td>`
      );
    });

    const avg = data.length === 0 ? 0 :
        Math.round(data.reduce((prev, elem) => {
          return prev + elem.latency;
        }, 0) / data.length * 100) / 100;

    this.tableHead.selectAll("#average-field").data([avg])
        .html(d => { return d; });

    if (isScrolledDown) {
      this.tableContainer.scrollTop = Number.MAX_SAFE_INTEGER;
    }

    return false;
  }

  filter(packets) {
    if (!this.filterBy) {
      return packets;
    }

    return packets.filter(packet => {
      return packet.from === this.filterBy;
    });
  }

  render() {
    return (
      <div className="table-container" ref="tableContainer">
        <table>
          <thead ref="tableHead">
            <tr>
              <th>From</th>
              <th>To</th>
              <th>Seq Num</th>
              <th>Latency</th>
            </tr>
            <tr>
              <td colSpan="3">Average:</td>
              <td id="average-field">50</td>
            </tr>
          </thead>
          <tbody ref="tableBody">
          </tbody>
        </table>
      </div>
    );
  }
}