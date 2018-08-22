import * as React from 'react';
import SearchBar from '../SearchBar/SearchBar'
import MethodList from '../MethodList/MethodList'
import { get } from '../../utils/Ajax'
import { GET_ALL_METHOD_IDS } from '../../config/config'
import ToggleButton from '../ToggleButton/ToggleButton'
import MethodSelect from '../MethodSelect/MethodSelect'

interface State {
    searchString: string,
    searchStringDf: string,
    displaySearchResults: boolean,
    results: { [methodId: string]: string },
    resultsDf: { [methodId: string]: string },
    methodMap: { [methodId: string]: string }
}

interface Props {
    searchForInstruction(searchString: string, cb: (result: { [methodId: string]: string }) => void): void,
    searchForDf(searchString: string, cb: (result: { [methodId: string]: string }) => void): void,
    resetSearch(resetType: number): void,
    graphId: string,
    selectMethod(methodId: string): void,
    showAllDfFacts(toggle: boolean): void,
    exportFullGraph: boolean,
    exportFullGraphToggle(toggle: boolean): void,
    showAllMethods(toggle: boolean): void,

    saveGraph(): void,
    showDfFactsToggle: boolean,
    showMethodsToggle: boolean,
    focusNextResult(type: number, next: boolean): void,
}
export default class ConfigBar extends React.Component<Props, State> {

    constructor(props: Props) {
        super(props);
        this.state = {
            searchString: '',
            searchStringDf: '',
            displaySearchResults: false,
            results: undefined,
            resultsDf: undefined,
            methodMap: undefined
        };

        this.handleSearchChange = this.handleSearchChange.bind(this);
        this.handleSearchSubmit = this.handleSearchSubmit.bind(this);
        this.handleSearchChangeDf = this.handleSearchChangeDf.bind(this);
        this.handleSearchSubmitDf = this.handleSearchSubmitDf.bind(this);
        this.resetSearch = this.resetSearch.bind(this);
        this.displayResults = this.displayResults.bind(this);
        this.displayResultsDf = this.displayResultsDf.bind(this);
        this.handleNextClick = this.handleNextClick.bind(this);
        this.handlePreviousClick = this.handlePreviousClick.bind(this);
        this.loadAllMethods();
    }

    loadAllMethods() {
        const url = GET_ALL_METHOD_IDS + this.props.graphId;
        get(url).then((data) => {
            this.setState({ methodMap: data });
        });
    }
    handleSearchChange(event: React.FormEvent<HTMLInputElement>) {
        this.setState({ searchString: event.currentTarget.value });
    }
    handleSearchChangeDf(event: React.FormEvent<HTMLInputElement>) {
        this.setState({ searchStringDf: event.currentTarget.value });
    }
    displayResults(result: { [methodId: string]: string }) {
        this.setState({ results: result });
    }
    displayResultsDf(result: { [methodId: string]: string }) {
        this.setState({ resultsDf: result });
    }

    handleSearchSubmitDf(event: React.FormEvent<HTMLFormElement>) {
        event.preventDefault();
        this.props.searchForDf(this.state.searchStringDf, this.displayResultsDf);
    }

    handleSearchSubmit(event: React.FormEvent<HTMLFormElement>) {
        event.preventDefault();
        this.props.searchForInstruction(this.state.searchString, this.displayResults);
    }
    resetSearch() {
        this.setState({ searchString: '', results: undefined });
        this.props.resetSearch(1);
    }
    resetSearchDf() {
        this.setState({ searchStringDf: '', resultsDf: undefined });
        console.log("reset clicked in component");
        this.props.resetSearch(5);
    }
    handleNextClick() {
        this.props.focusNextResult(1, true);
    }
    handlePreviousClick() {
        this.props.focusNextResult(1, false);
    }

    handleNextClickDf() {
        this.props.focusNextResult(5, true);
    }
    handlePreviousClickDf() {
        this.props.focusNextResult(5, false);
    }

    render() {
        let resultList = null;
        let methodList = null;
        let nextResultButton = null;
        let previousResultButton = null;
        if (this.state.results != undefined) {
            resultList = <MethodList methodMap={this.state.results} selectMethod={this.props.selectMethod}></MethodList>
            nextResultButton = <button type="button" onClick={this.handleNextClick}>Focus Next Result</button>
            previousResultButton = <button type="button" onClick={this.handlePreviousClick}>Focus Previous Result</button>
        }
        let resultListDf = null;
        let methodListDf = null;
        let nextResultButtonDf = null;
        let previousResultButtonDf = null;
        if (this.state.resultsDf != undefined) {
            resultListDf = <MethodList methodMap={this.state.resultsDf} selectMethod={this.props.selectMethod}></MethodList>
            nextResultButtonDf = <button type="button" onClick={this.handleNextClickDf.bind(this)}>Focus Next Result</button>
            previousResultButtonDf = <button type="button" onClick={this.handlePreviousClickDf.bind(this)}>Focus Previous Result</button>
        }
        if (this.state.methodMap != undefined) {
            methodList = <MethodSelect methodMap={this.state.methodMap} selectMethod={this.props.selectMethod}></MethodSelect>
        }
        return (
            <div style={{ display: 'grid', gridTemplateColumns: 'repeat(2, [col-start] 1fr)', gridGap: '10px' }}>
                <div style={{ gridColumn: 'col-start / span 2' }}>
                    <ToggleButton labelText={"Show All Data Flow Facts"} handleClick={this.props.showAllDfFacts} isToggleOn={this.props.showDfFactsToggle}></ToggleButton>
                </div>
                <div style={{ gridColumn: 'col-start / span 2' }}>
                    <ToggleButton labelText={"Show All Methods"} handleClick={this.props.showAllMethods} isToggleOn={this.props.showMethodsToggle}></ToggleButton>
                </div>
                <div style={{ gridColumn: 'col-start / span 2' }}>
                    <SearchBar resetSearch={this.resetSearch} labelTest={"Search Instruction:"} searchString={this.state.searchString} handleSearchChange={this.handleSearchChange} handleSearchSubmit={this.handleSearchSubmit}></SearchBar>
                </div>
                <div>
                    {previousResultButton}
                </div>
                <div>
                    {nextResultButton}
                </div>
                <div style={{ gridColumn: 'col-start / span 2' }}>
                    {resultList}
                </div>
                <div style={{ gridColumn: 'col-start / span 2' }}>
                    {methodList}
                </div>
                <div style={{ gridColumn: 'col-start / span 2' }}>
                    <SearchBar resetSearch={this.resetSearchDf.bind(this)} labelTest={"Search Data Flow Fact:"} searchString={this.state.searchStringDf} handleSearchChange={this.handleSearchChangeDf} handleSearchSubmit={this.handleSearchSubmitDf}></SearchBar>
                </div>
                <div>
                    {previousResultButtonDf}
                </div>
                <div>
                    {nextResultButtonDf}
                </div>
                <div style={{ gridColumn: 'col-start / span 2' }}>
                    {resultListDf}
                </div>

                <div style={{ gridColumn: 'col-start / span 1' }}>
                    <ToggleButton labelText={"Save Viewport Only"} handleClick={this.props.exportFullGraphToggle} isToggleOn={this.props.exportFullGraph}></ToggleButton>
                    <button type="button" onClick={this.props.saveGraph}>Save Graph as png</button>
                </div>
            </div >
        );
    }
}
