const svg = d3.select("svg"),
    width = +svg.attr("width"),
    height = +svg.attr("height");

let link = svg.append("g").attr("class", "links");
const links = [];
const neighbors = {};

let ipHop;

const sendTime = {};
const latencies = [];

const color = d3.scaleOrdinal(d3.schemeCategory20);

const simulation = d3.forceSimulation()
    .force("link", d3.forceLink().id(d => { return d.id; }))
    .force("charge", d3.forceManyBody())
    .force("center", d3.forceCenter(width / 2, height / 2));

const tooltip = d3.select("body").append("div")
    .attr("class", "tooltip")
    .style("opacity", 0);

d3.json("graphs/example.json", (error, graph) => {
  if (error) throw error;

  // const link = svg.append("g")
  //     .attr("class", "links")
  //     .selectAll("line")
  //     .data(graph.links)
  //     .enter().append("line")
  //     .attr("id", d => { return d.source + "-" + d.target; })
  //     .attr("stroke-width", d => { return d.value; });

  const node = svg.append("g")
      .attr("class", "nodes")
      .selectAll("circle")
      .data(graph.nodes)
      .enter().append("circle")
      .attr("id", d => { return d.id; })
      .attr("r", 5)
      .attr("fill", d => { return color(d.group); })
      .call(d3.drag()
          .on("start", dragstarted)
          .on("drag", dragged)
          .on("end", dragended))
      .on("mouseover", d => {
          tooltip.transition()
              .duration(200)
              .style("opacity", .9);
          tooltip.html(`${d.id} <br> rank: ${d.rank} <br> dc: ${d.dc}%`)
              .style("left", (d3.event.pageX + 10) + "px")
              .style("top", (d3.event.pageY - 28) + "px");
      })
      .on("mouseout", d => {		
          tooltip.transition()
            .duration(500)
            .style("opacity", 0);
      });

  ipHop = svg.insert("g", ":first-child")
      .attr("class", "ipHops")
      .selectAll("line")
      .data(graph.ipHops)
      .enter().append("line")
      .attr("id", d => { return d.id; })
      .attr("x1", d => { return d3.select("#" + d.source).attr("cx"); })
      .attr("y1", d => { return d3.select("#" + d.source).attr("cy"); })
      .attr("x2", d => { return d3.select("#" + d.target).attr("cx"); })
      .attr("y2", d => { return d3.select("#" + d.target).attr("cy"); })
      .style("opacity", 0);

  simulation
      .nodes(graph.nodes)
      .on("tick", ticked);

  simulation.force("link")
      .links(graph.links);

  function ticked() {
    d3.selectAll(".links").selectAll("line")
        .attr("x1", d => { return d3.select("#" + d.source).attr("cx"); })
        .attr("y1", d => { return d3.select("#" + d.source).attr("cy"); })
        .attr("x2", d => { return d3.select("#" + d.target).attr("cx"); })
        .attr("y2", d => { return d3.select("#" + d.target).attr("cy"); });

    d3.select(".ipHops").selectAll("line")
        .attr("x1", d => { return d3.select("#" + d.source).attr("cx"); })
        .attr("y1", d => { return d3.select("#" + d.source).attr("cy"); })
        .attr("x2", d => { return d3.select("#" + d.target).attr("cx"); })
        .attr("y2", d => { return d3.select("#" + d.target).attr("cy"); });

    node
        .attr("cx", d => { return d.x; })
        .attr("cy", d => { return d.y; });
  }
});

function dragstarted(d) {
  if (!d3.event.active) simulation.alphaTarget(0.3).restart();
  d.fx = d.x;
  d.fy = d.y;
}

function dragged(d) {
  d.fx = d3.event.x;
  d.fy = d3.event.y;
}

function dragended(d) {
  if (!d3.event.active) simulation.alphaTarget(0);
  d.fx = null;
  d.fy = null;
}

function colorNode(id) {
  d3.select("#node-" + id)
      .attr("fill", d => { return color(Math.random()); });
}

function highlightNode(id) {
  d3.select("#node-" + id)
      .transition()
        .duration(200)
        .attr("r", 10)
      .transition()
        .delay(600)
        .duration(500)
        .attr("r", 5);
}

function highlightLine(src, dst) {
  let from = src < dst ? src : dst;
  let to = src < dst ? dst : src;
  d3.select(`#node-${from}-node-${to}`)
      .transition()
        .duration(200)
        .attr("stroke-width", 3)
      .transition()
        .delay(600)
        .duration(500)
        .attr("stroke-width", 1);
}

function annotateRank(id, rank) {
  d3.select("#node-" + id)
      .each(d => {
        d.rank = rank;
      });
}

function annotateNeighbor(id, nbrId) {
  if (!neighbors[id]) {
    neighbors[id] = {};
  }
  if (!neighbors[id][nbrId]) {
    neighbors[id][nbrId] = 1;
    // check if there is a link in the opposite direction
    if (!neighbors[nbrId] || !neighbors[nbrId][id]) {
      let from = id < nbrId ? id : nbrId;
      let to = id < nbrId ? nbrId : id;
      links.push({
        "source": "node-" + from,
        "target": "node-" + to,
        "value": 1
      });

      link = d3.selectAll(".links")
          .selectAll("line")
          .data(links)
          .enter().append("line")
          .attr("id", d => { return d.source + "-" + d.target; })
          .attr("stroke-width", d => { return d.value; })
          .attr("x1", d => { return d3.select("#" + d.source).attr("cx"); })
          .attr("y1", d => { return d3.select("#" + d.source).attr("cy"); })
          .attr("x2", d => { return d3.select("#" + d.target).attr("cx"); })
          .attr("y2", d => { return d3.select("#" + d.target).attr("cy"); });
    }
  }
}

function packetSent(from, to, seqNum) {
  sendTime[`${from}-${to}-${seqNum}`] = window.performance.now();
}

function packetReceived(from, to, seqNum) {
  let time = window.performance.now();
  let latency = time - sendTime[`${from}-${to}-${seqNum}`];
  latencies.push(latency);
  let avg = latencies.reduce((a, b) => { return a + b }, 0) / latencies.length;
  
  console.log("Latency:", latency);
  d3.select("#last-latency").html(latency + " ms");
  d3.select("#avg-latency").html(avg + " ms");

  d3.select(".ipHops").select(`#hop-${to}-${from}`)
      .transition()
        .delay(500)
        .duration(500)
        .style("opacity", .6)
      .transition()
        .delay(1000)
        .duration(200)
        .style("opacity", 0);
}

function annotateDeliveryRatio(id, from, rec, exp) {
  d3.select("node-" + id)
      .each(d => {
        if (!d.dr) {
          d.dr = {};
        }
        d.dr[from] = {
          "rec": rec,
          "exp": exp
        }
      });
}

function annotateDutyCycle(id, dcOn, total) {
  d3.select("#node-" + id)
      .each(d => {
        d.dc = Math.round(dcOn / total * 10000) / 100;
      });
}

/* communication */

const connectButton = d3.select("#connect-button");
const ipField = d3.select("#ip-field");

connectButton.on("click", () => {
  const ip = ipField.property("value");
  connection.send(JSON.stringify({
    type: "connect",
    ip: ip
  }));
});

const connection = new WebSocket('ws://127.0.0.1:8001');

connection.onopen = () => {
  console.log('Connection open!');
}

connection.onerror = () => {
  console.log('Error occured!');
}

connection.onmessage = message => {
  const command = JSON.parse(message.data);
  switch (command.name) {
    case 'H':
      /* highlight command */
      highlightNode(command.id);
      break;
    case 'L':
      /* line command */
      highlightLine(parseInt(command.src), parseInt(command.dst));
      break;
    case 'R':
      /* rank command */
      annotateRank(command.id, parseInt(command.rank));
      break;
    case 'N':
      /* neighbor command */
      annotateNeighbor(command.id, command.nbrId);
      break;
    case 'P':
      /* packet command */
      if (command.mod === "sent") {
        packetSent(command.from, command.to, command.seqNum);
      } else {
        // rec
        packetReceived(command.from, command.to, command.seqNum);
      }
      break;
    case 'DR':
      annotateDeliveryRatio(command.id, command.from, parseInt(command.rec), parseInt(command.exp));
      break;
    case 'DC':
      /* duty cycle command */
      annotateDutyCycle(command.id, parseInt(command.dcOn), parseInt(command.total));
      break;
    default:
      console.log('Unknown command', command.name);
      break;
  }
}
