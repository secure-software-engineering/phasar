import * as express from 'express'
const router = express.Router();

router.get('*', function (req, res) {

  var html;
  if (process.env.NODE_ENV == 'production')
    html = `
    <!doctype html>
    <html>
      <head>
        <meta name="viewport" content="width=device-width, initial-scale=1" />
      </head>
      <body>
        <div id="app"></div>

        <script type="text/javascript" src="static/react.production.min.js"></script>
        <script type="text/javascript" src="static/react-dom.production.min.js"></script>
        <script type="text/javascript" src="static/cytoscape.min.js"></script>
        <script type="text/javascript" src="static/static.js"></script>
  
      </body>
    </html>  
  `
  else {
    html = `
    <!doctype html>
    <html>
      <head>
        <meta name="viewport" content="width=device-width, initial-scale=1" />
      </head>
      <body>
        <div id="app"></div>
        <script type="text/javascript" src="static/react.development.js"></script>
        <script type="text/javascript" src="static/react-dom.development.js"></script>
        <script type="text/javascript" src="static/static.js"></script>
      </body>
    </html>  
  `
  }
  res.send(html)
})

export default router
