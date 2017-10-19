import React from 'react';
import ReactDOM from 'react-dom';
import * as d3 from 'd3';

import './style/History.css'

export default class History extends React.Component {
  constructor(props) {
    super(props);


  }

  componentDidMount() {
    this.tableHead = d3.select(ReactDOM.findDOMNode(this.refs.tableHead));
    this.tableBody = d3.select(ReactDOM.findDOMNode(this.refs.tableBody));
  }

  shouldComponentUpdate(nextProps, nextState) {
    const rows = this.tableBody.selectAll("tr").data(nextProps.packets);
    rows.exit().remove();
    rows.enter().insert("tr", ":first-child")
        .html(d => {
          return (`
              <td>${d.from}</td>
              <td>${d.to}</td>
              <td>${d.seqNum}</td>
              <td>${Math.round(d.latency*100) / 100}</td>`
          );
        });

    const avg = Math.round(nextProps.packets.reduce((prev, elem) => {
      return prev + elem.latency;
    }, 0) / nextProps.packets.length * 100) / 100;

    this.tableHead.selectAll("#average-field").data([avg])
        .html(d => { return d; });

    return false;
  }

  render() {
    return (
      <div className="table-container">
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