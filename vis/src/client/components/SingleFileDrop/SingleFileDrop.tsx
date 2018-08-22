import * as React from 'react';
import { default as Dropzone, ImageFile, DropFilesEventHandler } from 'react-dropzone'
//import Dropzone = require("react-dropzone");


interface State {
    accpeted: ImageFile[],
    rejected: ImageFile[]
}

interface Props {
    handleDrop(file: File): void
}

export default class SingleFileDrop extends React.Component<Props, State> {
    constructor(props: Props) {
        super(props);
        this.state = {
            accpeted: undefined,
            rejected: undefined
        }

        this.handleDrop = this.handleDrop.bind(this);
    }

    handleDrop(accepted: ImageFile[], rejected: ImageFile[], event: React.DragEvent<HTMLDivElement>) {
        if (accepted.length == 0 || accepted.length > 1) {
            alert("Please only select one valid *.ll file");
        }
        else {
            this.setState({ accpeted: accepted });
            this.setState({ accpeted: accepted, rejected: rejected });
            this.props.handleDrop(accepted[0]);
        }
    }
    render() {
        let dropZoneContent: JSX.Element;
        if (this.state.accpeted == undefined || this.state.accpeted.length == 0) {
            dropZoneContent = <div><p>Drop your file here, or click to select a file to upload.</p><p>Only *.ll files will be accepted</p></div>
        }
        else {
            let fileName = this.state.accpeted[0].name;
            dropZoneContent = <div>{fileName}</div>
        }

        return (
            <div className="dropzone">

                {<Dropzone
                    accept=".ll, .cpp"
                    onDrop={this.handleDrop}
                >
                    {dropZoneContent}
                </Dropzone>}
            </div>
        );
    }
}