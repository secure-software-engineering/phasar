import * as React from 'react';

interface State {
    keys: Array<string>,
    value: string
}

interface Props {
    keyValueMap: { [key: string]: string },
    itemSelected(id: string): void

}
export default class Select extends React.Component<Props, State> {

    constructor(props: Props) {
        super(props);
        this.state = {
            keys: [],
            value: ''
        };

        for (let key in this.props.keyValueMap) {
            this.state.keys.push(key);
        }
        this.itemSelected = this.itemSelected.bind(this);
    }

    itemSelected(event: React.ChangeEvent<HTMLSelectElement>) {
        this.setState({ value: event.currentTarget.value });
        this.props.itemSelected(event.currentTarget.value);
    }

    render() {
        return (
            <select style={{ width: '90%' }} value={this.state.value} onChange={this.itemSelected}>
                {this.state.keys.map((id) =>
                    <option id={id} key={id} value={id}>
                        {this.props.keyValueMap[id]}
                    </option>
                )}
            </select>
        );
    }
}