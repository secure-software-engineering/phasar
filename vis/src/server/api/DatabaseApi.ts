import mongoose = require('mongoose');
import * as mongodb from 'mongodb';
const config = require('../config/config');
import debug = require('debug');
const dd = debug('database_api');
const de = debug('error');
import * as express from 'express'

mongoose.Promise = global.Promise;

var myDB: mongoose.Connection;

/**
 * Returns the database handle.
 * If hanlde is not initialized a Error is returned.
 */
export function getDB(): mongoose.Connection {
    if (myDB) {
        return myDB;
    }
    else {
        de("getDB was called but myDB is not initialized!");
    }
}

/**
 * Closes an open database connection. A callback may be provided to this method.
 */
export function close(callback: (err: Error) => void) {
    if (myDB) {
        myDB.close(function (err) {
            myDB = null
            callback(err)
        })
    }
}

/**
 * Connects to the database. 
 * Upon successfull connection the provided callback method is called. 
 * If connection is unsuccessfull the node application is closed since it 
 * cannot operate without the database. 
 */
export function connectToDatabase() {

    if (myDB) {
        return myDB;
    }

    myDB = mongoose.createConnection(config.DB_URL);
    dd("Database connected");
    return myDB
};

/**
 * Drops database.
 */
export function clearDatabase(req: express.Request, res: express.Response) {
    if (myDB) {
        myDB.dropDatabase(function (err: Error) {
            dd("dropped database");
            res.send({ "hello": "world" });
        });
    }
    res.send({ "hello": "world" });
}