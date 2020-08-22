const MyMongo = require('./mongo.js')
const Server = require('./server')

const myMongo = new MyMongo()
const server = new Server(myMongo)

async function init() {
  await myMongo.init()
  await server.init()
}
init()