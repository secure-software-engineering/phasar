import * as React from 'react';
import { render } from 'react-dom';
import { BrowserRouter as Router, Route, Link } from 'react-router-dom';
import App from './components/Index/Index';
import { MyRoute } from './interfaces/RoutingInterfaces';
import Home from './components/Home/Home';
import StartForm from './components/StartForm/StartForm';
import Database from './components/Database/Database';
import Result from './components/Result/Result';
import { setupPage, normalize } from "csstips";

const routes: Array<MyRoute> = [
  {
    path: '/',
    exact: true,
    main: () => <Home />,
    header: () => <h1>Home</h1>
  },
  {
    path: '/StartAnalysis',
    exact: false,
    main: () => <StartForm />,
    header: () => <h1>Start Analysis</h1>
  },
  {
    path: '/History',
    exact: false,
    main: () => <Route component={Database} />,
    header: () => <h1>Database</h1>
  },
  {
    path: '/Result',
    exact: false,
    main: () => <Route component={Result} />,
    header: () => <h1>Exploded Supergraph</h1>
  }
]
render((
  <App myRoutes={routes} />
), document.getElementById("app"));
normalize();
setupPage('#app');