import * as React from 'react'
import { Link, BrowserRouter as Router, Redirect } from 'react-router-dom'
import { postFile } from '../../utils/Ajax'
import { START_PROCESS } from '../../config/config'
import ToggleButton from '../ToggleButton/ToggleButton'
import Select from '../Select/Select'
import SingleFileDrop from '../SingleFileDrop/SingleFileDrop'


interface State {
    redirectToReferrer: boolean,
    file: File,
    wpa: boolean,
    moduleProject: string,
    mem2reg: boolean,
    name: string,
    framework: string,
    analysis: string,
    frameworkMap: { [id: string]: string },
    resultId: number,
    moduleProjectMap: { [id: string]: string },
    analysisMap: { [id: string]: string }
}

export default class StartForm extends React.Component<any, State> {
    constructor(props: any) {
        super(props);

        this.state = {
            redirectToReferrer: false,
            file: undefined,
            analysis: 'ifds-const',
            name: '',
            framework: 'p',
            wpa: true,
            mem2reg: false,
            moduleProject: 'm',
            resultId: -1,
            frameworkMap: { 'p': 'PHASAR' },
            moduleProjectMap: { 'm': 'module', 'p': 'project' },
            analysisMap: {  "ifds-const": "IFDS ConstAnalysis",
                            "ifds-lca": "IFDS LinearConstantAnalysis",
                            "ifds-solvertest": "IFDS SolverTest",
                            "ifds-taint": "IFDS TaintAnalysis",
                            "ifds-type": "IFDS TypeAnalysis",
                            "ifds-uninit": "IFDS UninitializedVariables",
                            "ide-lca": "IDE LinearConstantAnalysis",
                            "ide-solvertest": "IDE SolverTest",
                            "ide-taint": "IDE TaintAnalysis",
                            "ide-typestate": "IDE TypeStateAnalysis" },
        };
        this.redirectCallback = this.redirectCallback.bind(this);
    }

    redirectCallback(data: any) {
        // time stamp for finished analysis
        let finishedAnalysis: number = performance.now();
        console.log("analysis finished: ", finishedAnalysis);
        this.setState({ redirectToReferrer: true, resultId: data.resultId })
    };

    handleWpaClick(toggle: boolean) {
        this.setState({ wpa: toggle });
    }
    moduleProjectSelected(id: string) {
        this.setState({ moduleProject: id });
    }

    handleFrameworkSelect(framework: string) {
        this.setState({ framework: framework });
    }
    analysisSelected(id: string) {
        this.setState({ analysis: id });
    }
    handleDrop(file: File) {
        this.setState({ file: file });
    }
    handleMemRegClick(toggle: boolean) {
        this.setState({ mem2reg: toggle });
    }

    handleNameChange(changeEvent: React.ChangeEvent<HTMLInputElement>) {
        this.setState({ name: changeEvent.currentTarget.value });
    }

    submit(event: React.FormEvent<HTMLFormElement>) {
        event.preventDefault();
        let formData = new FormData();
        formData.append('name', this.state.name);
        formData.append('file', this.state.file);
        formData.append('mem2reg', this.state.mem2reg ? '1' : '0');
        formData.append('wpa', this.state.wpa ? '1' : '0');
        formData.append('analysis', this.state.analysis);
        formData.append('moduleProject', this.state.moduleProject);
        // Time Stamp for start analysis execution
        let startAnalysis = performance.now();
        console.log("analysis started: ", startAnalysis);
        postFile(START_PROCESS, formData).then(this.redirectCallback);
    }

    render() {

        if (this.state.redirectToReferrer) {
            return (
                <Redirect to={{
                    pathname: '/Result',
                    state: { "resultId": this.state.resultId }
                }} />
            )
        }
        else {
            return (
                <form onSubmit={this.submit.bind(this)}>
                    <div style={{ display: 'grid', gridTemplateColumns: '100px 200px', gridColumnGap: '10px', gridRowGap: '10px' }}>
                        <label>Name: </label><input type="text" value={this.state.name} onChange={this.handleNameChange.bind(this)} />
                        <label> Framework: </label>  <Select keyValueMap={this.state.frameworkMap} itemSelected={this.handleFrameworkSelect.bind(this)}></Select>
                        <label style={{ paddingTop: '8px' }}> WPA   </label><ToggleButton isToggleOn={this.state.wpa} handleClick={this.handleWpaClick.bind(this)}></ToggleButton>
                        <label style={{ paddingTop: '8px' }}> mem2reg</label><ToggleButton isToggleOn={this.state.mem2reg} handleClick={this.handleMemRegClick.bind(this)}></ToggleButton>
                        <label> Program Type</label>  <Select keyValueMap={this.state.moduleProjectMap} itemSelected={this.moduleProjectSelected.bind(this)}></Select>
                        <label> Analysis</label>  <Select keyValueMap={this.state.analysisMap} itemSelected={this.analysisSelected.bind(this)}></Select>
                        <label> Program to analyze</label>  <div style={{ gridColumnStart: 2, gridColumnEnd: 3 }}><SingleFileDrop handleDrop={this.handleDrop.bind(this)}></SingleFileDrop></div>
                        <input style={{ gridColumnStart: 2, gridColumnEnd: 3 }} type="submit" value="Start Analysis" />
                    </div>
                </form>

            );
        }

    }
}