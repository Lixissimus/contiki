import React from 'react';

import './style/Tile.css';

export default class Tile extends React.Component {
  render() {
    return (
      <div className="tile-container">
        <div className="title">
          <h2>{this.props.title}</h2>
        </div>
        <div className="content">
          {this.props.children}
        </div>
      </div>
    );
  }
}