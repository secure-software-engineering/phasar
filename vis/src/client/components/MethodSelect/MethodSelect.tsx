import * as React from 'react';
import Select from '../Select/Select'
interface State {
    methodIds: Array<string>,
    value: string
}

interface Props {
    methodMap: { [methodId: string]: string },
    selectMethod(methodId: string): void

}
export default class MethodSelect extends React.Component<Props, State> {

    constructor(props: Props) {
        super(props);
        this.state = {
            methodIds: [],
            value: ''
        };

        for (let key in this.props.methodMap) {
            this.state.methodIds.push(key);
        }
        this.itemSelected = this.itemSelected.bind(this);
        this.displayMethod = this.displayMethod.bind(this);
    }

    itemSelected(id: string) {
        this.setState({ value: id });
    }

    displayMethod(event: React.FormEvent<HTMLFormElement>) {
        event.preventDefault();
        this.props.selectMethod(this.state.value);
    }

    render() {
        return (
            <div>
                <form onSubmit={this.displayMethod}>
                    <label>
                        All Methods:
                        <Select keyValueMap={this.props.methodMap} itemSelected={this.itemSelected}></Select>
                    </label>
                    <input type="submit" value="Display Method" />
                </form>
                <ul style={{ width: '90%', maxHeight: '200px', overflow: 'hidden', overflowY: 'scroll' }}>

                </ul>
            </div>
        );
    }
}
