import * as React from 'react';
import {
  BrowserRouter as Router,
  Route,
  Link,
  RouteProps
} from 'react-router-dom';
import { style, media, types } from 'typestyle'
import { percent, px, em } from 'csx';
import MediaQuery from 'react-responsive';
import LargeSidebar from '../LargeSidebar/LargeSidebar'
import SmallSidebar from '../SmallSidebar/SmallSidebar'
import { MyRoute } from '../../interfaces/RoutingInterfaces';
// Based on https://reacttraining.com/react-router/web/example/sidebar


interface Props {
  myRoutes: Array<MyRoute>
}

class App extends React.Component<Props, any>{

  constructor(props: Props) {
    super(props);
  }

  // getContentStyle(): string {
  //   return style(
  //     media({ minWidth: 831 }, { marginLeft: px(200), padding: px(10) }),
  //     media({ maxWidth: 830 }, { marginLeft: px(50) }));
  // }
  // getHeaderStyle(): string {
  //   return style(
  //     media({ minWidth: 831 }, { marginLeft: px(200), paddingLeft: '5px', borderBottom: '1px solid lightGrey' }),
  //     media({ maxWidth: 830 }, { marginLeft: px(50), paddingLeft: '5px', borderBottom: '1px solid lightGrey' }));

  // }

  render() {

    // var contentStyle = this.getContentStyle();
    return (
      <Router >
        <div id="wrapper" style={{ display: 'grid', gridGap: '5px', gridTemplateColumns: 'repeat(12, [col-start] 1fr)' }}>

          {/* <MediaQuery minWidth={831}> */}
          <div style={{ gridColumn: 'col-start / span 1', gridRow: '1 / 4' }}>
            <LargeSidebar myRoutes={this.props.myRoutes}></LargeSidebar>
          </div>
          {/* </MediaQuery>
          <MediaQuery maxWidth={830} >
            <SmallSidebar myRoutes={this.props.myRoutes}></SmallSidebar>
          </MediaQuery > */}


          <div id="header" style={{ gridColumn: 'col-start 2 / span 11', borderBottom: '1px solid lightGrey', gridRow: '1 / 2' }}>

            {this.props.myRoutes.map((route, index) => (
              <Route
                key={index}
                path={route.path}
                exact={route.exact}
                component={route.header}
              />
            ))}

          </div>


          <div id="content" style={{ gridColumn: 'col-start 2 / span 11', gridRow: '2 / 4' }}>
            {this.props.myRoutes.map((route, index) => (
              <Route
                key={index}
                path={route.path}
                exact={route.exact}
                component={route.main}
              />
            ))}
          </div>

        </div>
      </Router >
    )
  }
}

export default App;