var path = require('path');
var webpack = require('webpack');
const CopyWebpackPlugin = require('copy-webpack-plugin');
const UglifyJSPlugin = require('uglifyjs-webpack-plugin');

module.exports = {

  plugins: [
    new CopyWebpackPlugin([
      { from: __dirname + '/node_modules/react/umd/react.production.min.js', to: __dirname + '/dist/static/' },
      { from: __dirname + '/node_modules/react-dom/umd/react-dom.production.min.js', to: __dirname + '/dist/static/' },
      { from: __dirname + '/node_modules/cytoscape/dist/cytoscape.min.js', to: __dirname + '/dist/static/' }
    ]),
    new webpack.DefinePlugin({
      'process.env': {
        NODE_ENV: JSON.stringify('production')
      }
    }),
    new UglifyJSPlugin()
  ],
  entry: ['./src/client/main.tsx'],
  output: { path: __dirname + '/dist', filename: 'static/static.js' },
  devtool: "source-map",
  resolve: {
    // Add '.ts' and '.tsx' as resolvable extensions.
    extensions: [".ts", ".tsx", ".js", ".json", ".svg", ".webpack.js"]
  },
  module: {
    rules: [
      // All files with a '.ts' or '.tsx' extension will be handled by 'awesome-typescript-loader'.
      { test: /\.tsx?$/, loader: "awesome-typescript-loader" },
      {
        test: /\.css$/,
        loader: 'style-loader!css-loader',
        exclude: /node_modules/
      },
      {
        test: /\.svg$/,
        loader: 'url-loader'
      },
    ]
  },
  // When importing a module whose path matches one of the following, just
  // assume a corresponding global variable exists and use that instead.
  // This is important because it allows us to avoid bundling all of our
  // dependencies, which allows browsers to cache those libraries between builds.
  externals: {
    "react": "React",
    "react-dom": "ReactDOM"
  },
};
