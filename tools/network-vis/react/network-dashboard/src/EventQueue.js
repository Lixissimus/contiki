export default class EventQueue {
  constructor() {
    this.subscribers = {};
  }

  post(type, data) {
    if (!this.subscribers[type]) {
      return;
    }

    this.subscribers[type].forEach(cb => {
      cb(data, type);
    }, this);
  }

  subscribe(type, callback) {
    if (!this.subscribers[type]) {
      this.subscribers[type] = [];
    }

    this.subscribers[type].push(callback);
  }
}
