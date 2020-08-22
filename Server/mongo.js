const MongoClient = require('mongodb').MongoClient
const url = 'set up your server link here'

const dbName = 'ESP'
function MyMongo() {
  let client
  let db
  this.init = async () => {
    client = await MongoClient.connect(url, { useNewUrlParser: true })
    db = client.db(dbName)
  }
  this.findOne = async (collectionName, query) => {
    let collection = db.collection(collectionName)
    return await collection.findOne(query)
  }
  this.findToArray = async (collection, query, options = {}) => {
    let cursor = db.collection(collection).find(query)
    if (options.sort) cursor = cursor.sort(options.sort)
    if (options.limit) cursor = cursor.limit(options.limit)
    if (options.skip) cursor = cursor.skip(options.skip)
    if (options.fields) cursor = cursor.project(options.fields)
    return cursor.toArray()
  }
  this.insertOne = async (collectionName, obj) => {
    let collection = db.collection(collectionName)
    return await collection.insertOne(obj)
  }

  this.updateOne = async (collectionName, query, update) => {
    let collection = db.collection(collectionName)
    return await collection.updateOne(query, update)
  }
  this.findOneAndDelete = async (collection, query, options) => {
    db.collection(collection).findOneAndDelete(query, options)
  }
}

module.exports = MyMongo
