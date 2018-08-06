var path = require('path');
var webpack = require('webpack');
const CopyWebpackPlugin = require('copy-webpack-plugin');

module.exports = {
  plugins: [
    new CopyWebpackPlugin([
      { from: __dirname + '/node_modules/react/umd/react.development.js', to: __dirname + '/dist/static/' },
      { from: __dirname + '/node_modules/cytoscape/dist/cytoscape.min.js', to: __dirname + '/dist/static/' },
      { from: __dirname + '/node_modules/react-dom/umd/react-dom.development.js', to: __dirname + '/dist/static/' }
    ])
  ],
  entry: ['./src/client/main.tsx'],
  output: { path: __dirname + '/dist/static', filename: 'static.js' },
  watch: true,
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
      // { test: /\.(jpe?g|gif|png|eot|svg|woff|woff2|ttf)$/, use: 'file-loader' },

      // All output '.js' files will have any sourcemaps re-processed by 'source-map-loader'.
      { enforce: "pre", test: /\.js$/, loader: "source-map-loader" }
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
