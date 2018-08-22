import * as React from 'react'

interface Props {
    name: string,
    backToMainMethod(): void
}
export default class BreadCrumbHeader extends React.Component<Props, any> {
    constructor(props: Props) {
        super(props);
    }

    render() {
        let button: JSX.Element = null;
        if (this.props.name != 'main') {
            button = <button onClick={this.props.backToMainMethod}>Back to main</button>
        }
        return (<div>
            {this.props.name}
            {button}
        </div>)
    }
}