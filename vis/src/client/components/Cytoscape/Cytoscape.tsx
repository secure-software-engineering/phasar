import * as React from 'react'
import { Link, Route } from 'react-router-dom'
import { get } from '../../utils/Ajax'
import * as cytoscape from 'cytoscape';
import { RESULT_URL, GET_METHOD_URL, GET_DFF_URL, SEARCH_STRING, GET_NEW_BASE_METHOD, GET_ALL_METHODS, SEARCH_STRING_DF } from '../../config/config';
import { CyMethod, CyEdge, CyNode, CytoscapeGraph } from '../../interfaces/CyModels'
import ConfigBar from '../ConfigBar/ConfigBar'
import BreadCrumbHeader from '../BreadCrumbHeader/BreadCrumbHeader'
import { saveAs } from 'file-saver'
import { RingList, RingListNode } from '../../utils/Ringlist'

interface State {
    resultId: string,
    cy: cytoscape.Core,
    resultUrl: string,
    removedNodes: RemovedMap,
    searchResult: cytoscape.NodeCollection,
    searchResultDf: cytoscape.NodeCollection,
    searchString: string,
    searchStringDf: string,
    graphEntryPoint: cytoscape.CollectionElements,
    displayedMethod: string,
    showAllDfFacts: boolean,
    showAllMethods: boolean,
    searchList: RingList<cytoscape.CollectionElements>,
    searchListDf: RingList<cytoscape.CollectionElements>,
    exportFullGraph: boolean
}
interface RemovedMap {
    [nodeId: string]: cytoscape.NodeCollection
}
interface Props {
    cyOptions: cytoscape.CytoscapeOptions
}

export default class Cytoscape extends React.Component<Props, State> {

    constructor(props: Props) {
        super(props);

        this.state = {
            resultId: history.state.state.resultId,
            resultUrl: RESULT_URL + history.state.state.resultId,
            cy: undefined,
            removedNodes: {},
            searchResult: undefined,
            searchString: '',
            exportFullGraph: false,
            searchResultDf: undefined,
            searchStringDf: '',
            searchList: new RingList<cytoscape.CollectionElements>(),
            searchListDf: new RingList<cytoscape.CollectionElements>(),
            graphEntryPoint: undefined,
            displayedMethod: 'main',
            showAllDfFacts: false,
            showAllMethods: false
        }

        this.searchForInstruction = this.searchForInstruction.bind(this);
        this.searchForDataflowfact = this.searchForDataflowfact.bind(this);
        this.resetSearch = this.resetSearch.bind(this);
        this.loadNewMethod = this.loadNewMethod.bind(this);
        this.backToMainMethod = this.backToMainMethod.bind(this);
        this.showAllDfFacts = this.showAllDfFacts.bind(this);
        this.expandCollapsedNode = this.expandCollapsedNode.bind(this);
        this.collapseExpandedNode = this.collapseExpandedNode.bind(this);
        this.showAllMethods = this.showAllMethods.bind(this);
        this.saveGraph = this.saveGraph.bind(this);
        this.exportFullGraphToggle = this.exportFullGraphToggle.bind(this);
        this.focusSearchResult = this.focusSearchResult.bind(this);

    }

    componentDidMount() {
        this.renderCytoscape();
    }

    layout() {
        let options: any = {
            name: 'dagre',
            rankSep: 10,
            nodeSep: 5,
            fit: false
        };
        var layout = this.state.cy.layout(options);

        return new Promise((resolve, reject) => {
            layout.run();
            resolve();
            let layoutFinished = performance.now();
            console.log("layout finished", layoutFinished);
            //timestamp for initial layout finished
        })
    }

    addInteractionListener() {
        this.addExpandListener();
        this.addCollapseListener();
    }

    addCollapseListener() {
        var collapsable = this.state.cy.nodes('node[collapsed = 0]');

        collapsable.on('cxttapend', (event: cytoscape.EventObject) => {

            let nodeId = event.target.id();
            let node = this.state.cy.getElementById(nodeId);
            if (node.data().collapsed == 0) {
                if (node.data().type == 4 && this.state.showAllMethods) {
                    this.setState({ showAllMethods: false });
                }
                if (this.state.showAllDfFacts && node.data().type == 6) {
                    this.setState({ showAllDfFacts: false });
                }
                this.collapseExpandedNode(node).then(() => {
                    this.layout();
                    this.addInteractionListener();
                });
            }
        });
    }

    collapseExpandedNode(node: cytoscape.CollectionElements): Promise<any> {

        node.data().collapsed = 1;
        //add children to map to restore them later
        let children = node.children().remove();
        let nodeId = node.data().id;

        let newObject: {
            [id: string]: cytoscape.CollectionElements
        } = {};
        newObject[nodeId] = children;
        return new Promise((resolve, reject) => {
            this.setState((prevState, props) => ({ removedNodes: Object.assign({}, prevState.removedNodes, newObject) }));
            resolve();
        });
    }

    loadDataflowFacts(dataflowFactNode: cytoscape.CollectionElements): Promise<any> {
        let nodeId = dataflowFactNode.data().from;
        let id: string = dataflowFactNode.data().id;

        let url = GET_DFF_URL + this.state.resultId + '&' + nodeId;

        return get(url).then((data) => {
            this.state.cy.add(data);
        });
    }

    loadMethod(methodNode: cytoscape.CollectionElements): Promise<any> {
        let methodId = methodNode.data().id;
        //request dataflow facts from server
        let url = GET_METHOD_URL + this.state.resultId + '&' + methodId;
        return get(url).then(async (data) => {
            let newElements = this.state.cy.add(data.nodes);
            this.state.cy.add(data.edges);

            if (this.state.showAllDfFacts) {
                let dfCompound = newElements.nodes('[type = 6]');
                await this.collapseExpandeToggle(true, dfCompound, false);
            }
            this.addToSearchResult(newElements);
        });
    }

    addExpandListener() {

        var expandable = this.state.cy.nodes('node[collapsed > 0]');

        expandable.on('click', (event: cytoscape.EventObject) => {
            let nodeId = event.target.id();
            let clickedNode = this.state.cy.getElementById(nodeId);
            expandable = expandable.difference(clickedNode);
            //sometimes click events are send twice, make soure to make server request only once
            if (clickedNode.data().collapsed > 0) {
                this.expandCollapsedNode(clickedNode).then((data) => {
                    this.layout();
                    this.addInteractionListener();
                });
            }
        });
    }

    expandCollapsedNode(clickedNode: cytoscape.CollectionElements): Promise<any> {
        if (clickedNode.data().collapsed == 2) {
            clickedNode.data().collapsed = 0;
            // expandable = this.state.cy.nodes('node[collapsed > 0]');
            switch (clickedNode.data().type) {
                case 6: //is dataflow compound node
                    return this.loadDataflowFacts(clickedNode);
                case 4: //is method node
                    return this.loadMethod(clickedNode);
            }
        }
        else {
            if (clickedNode.data().collapsed == 1) {

                clickedNode.data().collapsed = 0;
                let currentRemovedNodes = this.state.removedNodes
                let nodesToRestore = currentRemovedNodes[clickedNode.data().id];
                nodesToRestore.restore();
                this.addToSearchResult(nodesToRestore);
                delete currentRemovedNodes[clickedNode.data().id]
                return new Promise((resolve, reject) => {
                    this.setState((previousState) => ({ removedNodes: Object.assign({}, previousState.removedNodes, currentRemovedNodes) }), () => {
                    });
                    resolve();
                });
            }
        }
    }

    addToSearchResult(newNodes: cytoscape.NodeCollection) {

        const getNewSearchResults = (searchString: string, searchList: RingList<cytoscape.CollectionElements>, type: number): cytoscape.NodeCollection => {

            let searchResult = newNodes.nodes('node[name *= \"' + searchString + '\"][type =' + type + ']');

            searchResult.style('background-color', 'red');
            searchResult.forEach((node) => {
                searchList.add(node)
            });
            return searchResult;
        }

        if (this.state.searchString != '') {

            let searchResult = getNewSearchResults(this.state.searchString, this.state.searchList, 1);

            this.setState((previousState) => {
                if (previousState.searchResult != undefined)
                    return { searchResult: previousState.searchResult.add(searchResult) }
                else
                    return { searchResult: searchResult }
            });
        }
        if (this.state.searchStringDf != '') {
            let searchResult = getNewSearchResults(this.state.searchStringDf, this.state.searchListDf, 5);

            this.setState((previousState) => {
                if (previousState.searchResultDf != undefined)
                    return { searchResultDf: previousState.searchResultDf.add(searchResult) }
                else
                    return { searchResultDf: searchResult }
            });
        }
    }

    renderCytoscape() {
        //Timestamp for start initial layout
        let startInitialLayout = performance.now();

        console.log("start initial layout: ", startInitialLayout);
        var cydagre: any = require('cytoscape-dagre');

        cydagre(cytoscape); // register extension 

        this.props.cyOptions.container = document.getElementById('container');

        let cy = cytoscape(this.props.cyOptions);
        console.log("number of initial loaded nodes:", cy.nodes().size());
        this.setState({ cy: cy, graphEntryPoint: cy.elements(), }, () => { this.addInteractionListener(); this.layout(); });
    }

    resetSearch(resetType: number, cb?: () => any) {
        let searchResult;
        console.log("reste type", resetType);
        if (resetType == 1) {
            searchResult = this.state.searchResult;
        }
        else {
            searchResult = this.state.searchResultDf;
        }
        if (searchResult != undefined) {
            this.state.cy.nodes().style("background-color", function (ele: any) {
                let type = ele.data().type;
                let collapsed = ele.data().collapsed;
                if (type == 4 && collapsed != undefined && collapsed != 0) {
                    return "yellow";
                }
                else {
                    if (type != resetType)
                        return ele.style().backgroundColor;
                    else
                        return 'white'
                }
            });
            if (cb) {
                if (resetType == 1)
                    this.setState({ searchResult: undefined, searchList: new RingList<cytoscape.CollectionElements>(), searchString: '' }, cb());
                else
                    this.setState({ searchResultDf: undefined, searchListDf: new RingList<cytoscape.CollectionElements>(), searchStringDf: '' }, cb());
            }
            else
                if (resetType == 1)
                    this.setState({ searchResult: undefined, searchList: new RingList<cytoscape.CollectionElements>(), searchString: '' });
                else
                    this.setState({ searchResultDf: undefined, searchListDf: new RingList<cytoscape.CollectionElements>(), searchStringDf: '' });
        }
        else {
            if (cb)
                cb();
        }
    }

    focusSearchResult(type: number, next: boolean) {

        let getNode: () => RingListNode<cytoscape.CollectionElements>;
        let searchList: RingList<cytoscape.CollectionElements>;

        if (type == 1) {
            searchList = this.state.searchList;
        }
        else {
            searchList = this.state.searchListDf;
        }

        if (next)
            getNode = searchList.next;
        else
            getNode = searchList.previous;

        let currentSearchNode = getNode();
        const startNode = currentSearchNode;
        do {
            if (!currentSearchNode.getData().removed()) {
                break;
            }
            currentSearchNode = getNode();
        } while (startNode != currentSearchNode)

        if (currentSearchNode.getData().removed()) {
            alert("No search result visible in this method");
        }

        else {
            this.state.cy.animate({
                fit: {
                    eles: currentSearchNode.getData(),
                    padding: 200
                },
                duration: 700,
                queue: true
            });
        }
    }

    searchForDataflowfact(searchString: string, cb: (result: { [methodId: string]: string }) => void): void {
        event.preventDefault();
        //timestamp local search start
        let startLocalSearch = performance.now();
        console.log("started local search", startLocalSearch);


        this.resetSearch(5, () => {

            this.setState({ searchStringDf: searchString }, () => {
                this.addToSearchResult(this.state.cy.nodes());
            });
            //timestamp local search end
            let endLocalSearch = performance.now();
            console.log("end local search", endLocalSearch - startLocalSearch);

            let url = SEARCH_STRING_DF + this.state.resultId + '&' + searchString;
            //timestamp server search
            this.searchForStringServer(url, cb)
        });
    }

    searchForStringServer(url: string, cb: (result: { [methodId: string]: string }) => void) {
        let endLocalSearch = performance.now();
        get(url).then((data) => {
            cb(data);
            //timestamp server search  finish
            let endServerSearch = performance.now();
            if (endLocalSearch)
                console.log("finished server search", endServerSearch - endLocalSearch);
        });
    }


    searchForInstruction(searchString: string, cb: (result: { [methodId: string]: string }) => void): void {
        event.preventDefault();
        //timestamp local search start
        let startLocalSearch = performance.now();
        console.log("started local search", startLocalSearch);


        this.resetSearch(1, () => {

            this.setState({ searchString: searchString }, () => {
                this.addToSearchResult(this.state.cy.nodes());
            });
            //timestamp local search end
            let endLocalSearch = performance.now();
            console.log("end local search", endLocalSearch - startLocalSearch);

            let url = SEARCH_STRING + this.state.resultId + '&' + searchString;
            //timestamp server search
            this.searchForStringServer(url, cb)
        });
    }

    loadNewMethod(methodId: string) {
        const url = GET_NEW_BASE_METHOD + this.state.resultId + '&' + methodId;
        console.log("started load new mehtod: ", performance.now());
        get(url).then((data) => {
            let mainGraph = this.state.cy.elements().remove();
            console.log("added nodes: ", data.method.nodes.length);
            this.state.cy.add(data.method.nodes);
            this.state.cy.add(data.method.edges);
            this.setState({ displayedMethod: data.method_name });
            this.addInteractionListener();
            this.layout();
            this.addToSearchResult(this.state.cy.nodes())
        });
    }

    backToMainMethod() {
        if (this.state.graphEntryPoint != undefined) {
            this.state.cy.elements().remove();
            this.state.graphEntryPoint.restore();
            //reset node style
            this.state.graphEntryPoint.style("background-color", function (ele: any) {
                let type = ele.data().type;
                let collapsed = ele.data().collapsed;
                if (type == 4 && collapsed != undefined && collapsed != 0) {
                    return "yellow";
                }
                else {
                    return 'white'
                }
            });
            this.addToSearchResult(this.state.cy.nodes());
            this.setState({ displayedMethod: 'main' });
        }
    }

    showAllDfFacts(toggle: boolean) {
        this.setState({ showAllDfFacts: toggle }, async () => {
            let startShowAllDfFacts = performance.now();

            //remove await if not measuring time !!!
            let toExpand = this.state.cy.nodes('[type = 6][collapsed]');
            console.log("number of dffacts to expand: ", toExpand.size());
            await this.collapseExpandeToggle(toggle, toExpand, true);
            let finishedShowAllDfFacts = performance.now();
            console.log("finished show all df facts", finishedShowAllDfFacts - startShowAllDfFacts);
        });
    }

    showAllMethods(toggle: boolean) {
        this.setState({ showAllMethods: toggle, showAllDfFacts: false }, () => {
            var request1 = performance.now();
            //console.log("started show all method request", request1);
            if (toggle)
                get(GET_ALL_METHODS + this.state.resultId).then((data) => {
                    //update node data !
                    var request2 = performance.now();
                    console.log("received all method response", request2 - request1);
                    console.log("start add new nodes and relayout", request2);
                    // console.log("request time all methods: ", request2 - request1);
                    // console.log("received all method response", data);
                    let newNodes = this.state.cy.add(data.nodes);
                    this.addToSearchResult(newNodes);
                    console.log("number of nodes loaded: ", data.nodes.length);
                    console.log("number of edges loaded: ", data.edges.length);
                    this.state.cy.add(data.edges);
                    this.state.cy.nodes().style('background-color', 'white');

                    this.layout();
                    this.addInteractionListener();
                });
            else {

                this.state.cy.elements().remove();
                this.state.graphEntryPoint.restore();
                let dfCluster = this.state.cy.nodes('[type = 6]');

                dfCluster.forEach((node: any) => {
                    console.log(node);
                    node.data().collapsed = 2;
                })
                this.layout();
                this.addInteractionListener();

            }
        });
    }

    collapseExpandeToggle(toggle: boolean, nodes: cytoscape.NodeCollection, relayout: boolean): Promise<any> {
        if (nodes.length > 0) {
            var method: (clickedNode: cytoscape.CollectionElements) => Promise<any>;

            if (toggle)
                method = this.expandCollapsedNode;
            else
                method = this.collapseExpandedNode;

            let allRequests: Promise<any>[] = [];

            nodes.forEach((node) => {

                allRequests.push(method(node));
            });
            return Promise.all(allRequests).then(() => {
                if (relayout)
                    this.layout();
                this.addInteractionListener();
            });
        }
        else {
            console.log("unneccessarry");
        }
    }

    exportFullGraphToggle(toggle: boolean) {
        this.setState({ exportFullGraph: toggle });
    }

    saveGraph() {

        let png: Blob = this.state.cy.png({ full: !this.state.exportFullGraph, output: 'blob' }) as any as Blob;

        let fileName = this.state.resultId + "_" + Date.now();
        saveAs(png, fileName);
    }
    render() {
        return (
            <div id="resultWrapper" style={{ display: 'grid', gridTemplateColumns: 'repeat(12, [col-start] 1fr)' }}>
                <div id="breadCrumbHeader" style={{ gridColumn: 'col-start / span 12', gridRow: '1 / 2' }}>
                    <BreadCrumbHeader name={this.state.displayedMethod} backToMainMethod={this.backToMainMethod}></BreadCrumbHeader>
                </div>
                <div id="cyContent" style={{ gridColumn: 'col-start / span 10', gridRow: '2 / 4' }}>
                    <div id="container" style={{
                        layout: 'absolute',
                        left: 0,
                        height: '100vh',
                        width: '100%'
                    }}></div>
                </div>
                <div id="configBar" style={{ gridColumn: 'col-start 11 / span 2', gridRow: '2 / 4' }}>
                    <ConfigBar focusNextResult={this.focusSearchResult} exportFullGraphToggle={this.exportFullGraphToggle} selectMethod={this.loadNewMethod} exportFullGraph={this.state.exportFullGraph} saveGraph={this.saveGraph} showDfFactsToggle={this.state.showAllDfFacts} showMethodsToggle={this.state.showAllMethods} showAllMethods={this.showAllMethods} showAllDfFacts={this.showAllDfFacts} resetSearch={this.resetSearch} searchForDf={this.searchForDataflowfact} searchForInstruction={this.searchForInstruction} graphId={this.state.resultId} ></ConfigBar>
                </div>
            </div>
        );
    }

}