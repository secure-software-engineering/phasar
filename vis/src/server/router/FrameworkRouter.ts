import * as express from 'express';
const router = express.Router();
import * as  frameworkApi from '../api/FrameworkApi.js';
import * as  multer from 'multer'
import * as path from 'path'

const uploadPath = path.join(process.cwd(), "server/data/uploads/");

var storage = multer.diskStorage({
    destination: function (req, file, cb) {
        cb(null, uploadPath);
    },
    filename: function (req, file, cb) {
        cb(null, file.fieldname + Date.now() + '.json');
    }
})

const upload = multer({ storage: storage })
router.post('/addGraph', upload.single('sendfile'), frameworkApi.addToGraph);

export default router;