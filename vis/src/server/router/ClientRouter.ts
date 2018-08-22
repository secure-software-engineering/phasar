import * as express from 'express';
import * as  multer from 'multer'
import { UPLOAD } from '../config/config';

import * as dbApi from '../api/DatabaseApi';
import * as clientApi from '../api/ClientApi';
import * as path from 'path'
import * as fs from 'fs';
const router = express.Router();

const uploadPath = path.join(process.cwd(), "server/data/uploads/");

var storage = multer.diskStorage({
    destination: function (req, file, cb) {
        cb(null, uploadPath);
    },
    filename: function (req, file, cb) {
        let postfix;
        if (file.originalname.includes('.cpp')) {
            postfix = '.cpp'
        }
        else {
            postfix = '.ll'
        }

        cb(null, file.fieldname + Date.now() + postfix);
    }
})

const upload = multer({ storage: storage })



router.post('/startProcess', upload.single('file'), clientApi.startProcess);

router.get('/getGraphIds', clientApi.getGraphIds);
router.get('/dropDatabase', dbApi.clearDatabase);
router.get('/baseMethod/:graphId&:methodId', clientApi.newBaseMethod);
router.get('/searchInstruction/:graphId&:searchString', clientApi.searchInstructionString)
router.get('/searchDf/:graphId&:searchString', clientApi.searchDfString)
router.get('/result/:resultId', clientApi.getAnalysisResult);
router.get('/method/:graphId&:methodId', clientApi.getMethod);
router.get('/dataflowFacts/:graphId&:nodeId', clientApi.getDataflowFacts);
router.get('/method/all/:graphId', clientApi.getAllMethodIds);
router.get('/method/get/all/:graphId', clientApi.getAllMethods);
export default router;