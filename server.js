const express = require('express')
const bodyParser = require('body-parser')
const jwt = require('jsonwebtoken')
const R = require('ramda')
const path = require('path')
const argon2 = require('argon2')
const buildDirectory = '/../public'
ObjectId = require('mongodb').ObjectID
function Server(myMongo) {
  const app = express()
  const cors = require('cors')
  app.use(bodyParser.urlencoded({ extended: false }))

  app.use(bodyParser.json())

  app.use(express.static(path.join(__dirname, buildDirectory)))

  const port = process.env.PORT || 5002
  const tokenPassword = 'haslohaslohaslohaslohaslo'

  this.init = () => {
    app.listen(port, () => console.log(`App listening on http://127.0.0.1:${port}`))
  }
  const omitId=R.omit(['_id'])
  app.use(cors())

  // /config?temp=30.5&wilg=5.5
  // /reading?temp=30.5&wilg=10.4
  // /stateChange?grzalka=1&pompka=0&date=2020.10.04

  app.get('/config', async (req, res) => {
    res.send({ temp: '30.5', wilg: '10.5' })
  })

  app.get('/reading', async (req, res) => {
    res.send({ temp: '30.5', wilg: '10.5' })
  })

  app.get('/stateChange', async (req, res) => {
    res.send({ temp: '30.5', wilg: '10.5' })
  })

  app.get('/getConfig', async (req, res) => {
    res.send(omitId(await myMongo.findOne('config', { _id: ObjectId('5e8cc6011c9d44000028f3a8') })))
  })

  app.get('/getReadings', async (req, res) => {
    res.send(R.map(omitId, await myMongo.findToArray('readings', {})))
  })

  app.get('/getStateChanges', async (req, res) => {
    res.send(R.map(omitId, await myMongo.findToArray('states', {})))
  })

  app.get('/*', async (req, res) => {
    res.send({ hello: 'world' })
  })
}
module.exports = Server
