import * as React from 'react'
import { Link, Route } from 'react-router-dom'
import { get } from '../../utils/Ajax'
import * as cytoscape from 'cytoscape';
import { RESULT_URL, GET_METHOD_URL } from '../../config/config';
import Cytoscape from '../Cytoscape/Cytoscape'

interface State {
    loading: number,
    resultId: string,
    resultUrl: string,
    cyOptions: cytoscape.CytoscapeOptions
}

export default class Result extends React.Component<any, State> {

    constructor(props: any) {
        super(props);

        this.state = {
            loading: 1,
            resultId: history.state.state.resultId,
            resultUrl: RESULT_URL + history.state.state.resultId,
            cyOptions: undefined
        }

        this.renderCytoscape = this.renderCytoscape.bind(this);

        get(this.state.resultUrl).then(this.renderCytoscape);
    }


    renderCytoscape(data: any) {

        let options: cytoscape.CytoscapeOptions = {
            container: undefined,

            boxSelectionEnabled: false,
            autounselectify: true,
            layout: {
                name: 'dagre'
            },
            "style": [{
                "selector": "node",
                "css": {
                    "background-opacity": 1.0,
                    "font-size": 12,
                    "text-valign": "center",
                    "text-halign": "center",
                    "font-family": "SansSerif",
                    "font-weight": "normal",
                    "border-color": "black",
                    "border-opacity": 1.0,
                    "border-width": function (ele: any) {
                        let type = ele.data().type;
                        let collapsed = ele.data().collapsed
                        if (type == 4 || type == 1 || (type == 6 && collapsed != 0) || type == 5)
                            return 1.0
                        else
                            return 0.0
                    },
                    "width": 'label',
                    "shape": function (ele: any) {
                        let type = ele.data().type
                        if (type == 1)
                            return "roundrectangle"
                        if (type == 4)
                            return 'rectangle'
                        if (type == 5)
                            return "ellipse"
                        return "ellipse"
                    },
                    "color": "rgb(0,0,0)",
                    "height": 35.0,
                    "text-opacity": 1.0,
                    "background-color": function (ele: any) {
                        let type = ele.data().type;
                        let collapsed = ele.data().collapsed;
                        if (type == 4 && collapsed != undefined && collapsed != 0) {
                            return "yellow";
                        }
                        return 'white'
                    },
                    "content": "data(name)"
                }
            },
            {
                "selector": "node:selected",
                "css": {
                    "background-color": "rgb(255,255,0)"
                }
            }, {
                "selector": "edge",
                "css": {
                    "target-arrow-color": "rgb(0,0,0)",
                    "opacity": 1.0,
                    "source-arrow-shape": "none",
                    "target-arrow-shape": "none",
                    "width": 2.0,
                    "color": "rgb(0,0,0)",
                    "font-family": "Dialog",
                    "font-weight": "normal",
                    "source-arrow-color": "rgb(0,0,0)",
                    "line-color": "rgb(132,132,132)",
                    "text-opacity": 1.0,
                    "line-style": "solid",
                    "font-size": 10,
                    "content": ""
                }
            }, {
                "selector": "edge:selected",
                "css": {
                    "line-color": "rgb(255,0,0)"
                }
            }],
            "elements": { "nodes": data.method.nodes, "edges": data.method.edges }
        };

        this.setState({ cyOptions: options, loading: 0 });
    }


    render() {
        if (this.state.loading) {
            return (<div> Loading...</div>);
        }
        else {
            return (

                <Cytoscape cyOptions={this.state.cyOptions}></Cytoscape>

            );
        }
    }
}
