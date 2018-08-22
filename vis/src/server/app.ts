import * as express from 'express'
import * as socketio from 'socket.io'
import * as http_lib from 'http'
import * as bodyParser from 'body-parser'
import * as helmet from 'helmet'
import * as mongod from 'mongodb'
import * as mongoose from 'mongoose'

const app = express();
const http = new http_lib.Server(app);
const io = socketio(http);

import * as dbApi from './api/DatabaseApi'
import * as config from './config/config'

dbApi.connectToDatabase();
app.use("/static", express.static('dist/static'));
app.use(helmet());
app.use(bodyParser.json())
import { default as FrameworkRouter } from './router/FrameworkRouter';
import { default as ClientRouter } from './router/ClientRouter'
import { default as Router } from './router/index'
app.use('/api/framework', FrameworkRouter);
app.use('/api/client', ClientRouter);

app.use('*', Router);

app.listen(config.PORT, function () {
  console.log('Example app listening on port 3000!')
});

