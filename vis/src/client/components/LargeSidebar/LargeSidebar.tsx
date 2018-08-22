import {
    BrowserRouter as Router,
    Route,
    Link
} from 'react-router-dom';
import { style, media, types } from 'typestyle'
import { percent, px, em } from 'csx';
import * as React from 'react';
import { MyRoute } from '../../interfaces/RoutingInterfaces';

const logo = require('./../../img/fraunhofer/Heinz_Nixdorf_Institut_Logo_CMYK.svg');
const homeLogo = require('./../../img/materialDesign/home.svg');
const broker = require('./../../img/cytoscape/broker.svg');
const robot = require('./../../img/cytoscape/robot.svg');


interface Props {
    myRoutes: Array<MyRoute>
}

class LargeSidebar extends React.Component<Props, any>{
    constructor(props: Props) {
        super(props);
    }
    getLargeNavbarStyle(): string {

        var largerStyle: types.NestedCSSProperties = {
            margin: 0,
            padding: 0,
            top: 0,
            backgroundColor: 'white',
            width: percent(100),
            //  maxWidth: px(200),
            height: percent(100), /* Full height */
            //position: 'fixed', /* Make it stick, even on scroll */
            //overflow: 'auto' /* Enable scrolling if the sidenav has too much content */,
            borderRight: '1px solid lightGrey',
            $nest: {
                '& div': {

                    width: percent(100),
                    borderBottom: '1px solid lightGrey',
                    $nest: {
                        '& img': {
                            width: percent(100)

                        },
                        ['& p']: {
                            color: 'grey',
                            fontWeight: 'lighter',
                            fontSize: em(0.875)
                        }
                    },
                },
                '& ul': {
                    margin: 0,
                    padding: 0,
                    listStyleType: 'none',

                    $nest: {
                        '&>*:first-child': { marginTop: px(16) },
                        '& li': {
                            $nest: {

                                '& a': {
                                    $nest: {
                                        '&:hover': {
                                            backgroundColor: '#003A80',
                                            color: 'white'
                                        },
                                        '& img': {
                                            paddingRight: px(16),
                                            width: px(40),
                                            verticalAlign: 'text-bottom'
                                        },
                                    },
                                    display: 'block',
                                    color: '#000',
                                    padding: '8px 16px',
                                    textDecoration: 'none',
                                },
                            },
                        }
                    },
                },
            }
        };

        return style(largerStyle);
    }

    render() {
        return (
            <div className={this.getLargeNavbarStyle()} id="mySidebar" >
                <div>
                    <img src={logo} alt="logo" />
                </div>
                <ul>
                    <li><Link to="/"><img src={homeLogo}></img>Home</Link></li>
                    <li ><Link to="/StartAnalysis"><img src={robot}></img>Start Analysis</Link></li>
                    <li ><Link to="/History"><img src={broker}></img>Database</Link></li>
                </ul>
                {this.props.myRoutes.map((route, index) => (
                    <Route
                        key={index}
                        path={route.path}
                        exact={route.exact}
                    />
                ))}
            </div>);
    }
}

export default LargeSidebar;