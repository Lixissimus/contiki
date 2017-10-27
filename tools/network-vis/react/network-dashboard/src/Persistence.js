import React from 'react';

import './style/Persistence.css'

export default class Persistence extends React.Component {
  constructor(props) {
    super(props);
    this.downloadLink = null;
    this.fileInput = null;
    this.ipInput = null;
  }

  load() {
    if (!this.fileInput.files.length) {
      console.log("no file");
      return;
    }

    const reader = new FileReader();
    reader.onload = () => {
      try {
        const json = JSON.parse(reader.result);
        this.props.post("set-history", { history: json });
      } catch (error) {
        alert("Error parsing JSON file");
      }
    }
    reader.readAsText(this.fileInput.files[0]);
  }

  save() {
    const dataString = JSON.stringify(this.props.getData());
    const blob = new Blob([dataString], { type: "application/json" });
    const url = URL.createObjectURL(blob);
    this.downloadLink.download = "history.json";
    this.downloadLink.href = url;
    this.downloadLink.style = "display: null";
  }

  connect() {
    this.props.post("connect-ws", { url: this.ipInput.value });
  }

  render() {
    return (
      <div>
        <div style={{display: "block"}}>
          <input
              ref={comp => { this.fileInput = comp }}
              type="file" 
              id="fileInput" />
          <button 
              className="persistence button"
              onClick={this.load.bind(this)} >
            Load
          </button>
          <button 
              className="persistence button"
              onClick={this.save.bind(this)} >
            Persist
          </button>
          <a ref={comp => { this.downloadLink = comp }} style={{display: "none"}} >Download</a>
        </div>
        <div style={{display: "block"}}>
          <input
              ref={comp => { this.ipInput = comp }}
              type="text"
              id="ipInput"
              defaultValue="127.0.0.1:8001" />
          <button
              className="persistence button"
              onClick={this.connect.bind(this)} >
            Connect
          </button>
        </div>
      </div>
    );
  }
}
