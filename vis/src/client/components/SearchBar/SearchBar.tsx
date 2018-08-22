import * as React from 'react';

interface Props {
    searchString: string,
    handleSearchChange(event: React.FormEvent<HTMLInputElement>): void,
    handleSearchSubmit(event: React.FormEvent<HTMLFormElement>): void,
    resetSearch(): void,
    labelTest: string
}
export default class SearchBar extends React.Component<Props, any> {

    constructor(props: Props) {
        super(props);

    }


    render() {
        return (
            <div>
                <form onSubmit={this.props.handleSearchSubmit}>
                    <label>
                        {this.props.labelTest}
                        <input type="text" value={this.props.searchString} onChange={this.props.handleSearchChange} />
                    </label>

                    <input type="submit" value="Submit" />
                    <button type="button" onClick={this.props.resetSearch}>Reset Search</button>

                </form>
            </div>
        );
    }
}
