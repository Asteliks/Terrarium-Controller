const express = require('express')
const bodyParser = require('body-parser')
const jwt = require('jsonwebtoken')
const R = require('ramda')
const path = require('path')
ObjectId = require('mongodb').ObjectID
function Server(myMongo) {
  const app = express()
  const cors = require('cors')

  app.use(bodyParser.urlencoded({ extended: false }))

  app.use(bodyParser.json())

  const port = process.env.PORT || 5002

  this.init = () => {
    app.listen(port, () => console.log(`App listening on http://127.0.0.1:${port}`))
  }
  const omitId = R.omit(['_id'])
  const configId = '5e8cc6011c9d44000028f3a8' //whardkodowane, bo czemu nie
  const configQuery = { _id: ObjectId(configId) }
  app.use(cors())

  app.get('/config', async (req, res) => {
    const temp = req.query.temp
    const wilg = req.query.wilg
    const config = await myMongo.findOne('config', configQuery)
    await myMongo.updateOne('config', configQuery, {
      $set: {
        ...config,
        ...(wilg ? { wilg } : {}),
        ...(temp ? { temp } : {}),
      },
    })
    const current = omitId(await myMongo.findOne('config', configQuery))
    res.send(current)
  })

  app.get('/reading', async (req, res) => {
    const temp = req.query.temp
    const wilg = req.query.wilg
    if (temp && wilg) {
      await myMongo.insertOne('readings', { wilg, temp, date: new Date() })
      res.send(R.map(omitId, await myMongo.findToArray('readings', {})))
    } else {
      res.send({ error: 'error' })
    }
  })

  app.get('/stateChange', async (req, res) => {
    const pompka = req.query.pompka
    const grzalka = req.query.grzalka
    if (pompka && grzalka) {
      await myMongo.insertOne('states', { grzalka, pompka, date: new Date() })
      res.send(R.map(omitId, await myMongo.findToArray('readings', {})))
    } else {
      res.send({ error: 'error' })
    }
  })

  app.get('/getConfig', async (req, res) => {
    res.send(omitId(await myMongo.findOne('config', configQuery)))
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
