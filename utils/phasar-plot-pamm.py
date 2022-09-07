#!/usr/bin/env python3

# author: Richard Leer

# For DataFrame please refer to https://pandas.pydata.org/pandas-docs/stable/generated/pandas.DataFrame.plot.html
# For colormap please refer to https://matplotlib.org/users/colormaps.html

import json
import pprint
import numpy as np
import pandas
import sys
import getopt
import matplotlib.pyplot as plt
from matplotlib.gridspec import GridSpec


def dfToNumeric(df):
    for column in df:
        df[column] = pandas.to_numeric(df[column])


def drawHistogram(df, ax, x, y, title):
    df.plot(kind='bar', ax=ax, x=x, y=y, color='k', alpha=0.8,
            width=1, rot=90, legend=False, logy=True)
    ax.set_xlabel("")
    ax.set_ylabel("#Occurrences")
    ax.set_title(title)


def drawCounter(df, ax, x, y, plt, title):
    df.plot(kind='bar', color='green', ax=ax, x=x, y=y, fontsize=8,
            logy=False, legend=False, rot=20, alpha=0.8)
    ax.set_xlabel("")
    ax.set_ylabel("")
    ax.grid('on', which='major', axis='y', linestyle='-', linewidth=0.5)
    plt.setp(ax.xaxis.get_majorticklabels(), ha='right')
    ax.set_title(title)


def drawTimer(df, ax, x, y, plt, title):
    df.plot.bar(ax=ax, x=x, y=y, color='orange', fontsize=8,
                logy=False, legend=False, rot=20, alpha=0.8)
    ax.set_xlabel("")
    ax.set_ylabel("Time (sec)")
    ax.grid('on', which='major', axis='y', linestyle='-', linewidth=0.5)
    ax.set_title(title)
    plt.setp(ax.xaxis.get_majorticklabels(), ha='right')


def main(argv):
    path_to_json_file = ''
    try:
        opts, args = getopt.getopt(argv, "hi:", ["ifile="])
    except getopt.GetoptError:
        print("Usage: plot_pamm_results.py -i path/to/json/file")
        sys.exit(2)
    for opt, arg in opts:
        if opt == '-h' or opt == '':
            print("Usage: plot_pamm_results.py -i path/to/json/file")
            sys.exit()
        elif opt in ("-i", "--ifile"):
            path_to_json_file = arg

    with open(path_to_json_file) as file:
        data = json.load(file)
    pprint.pprint(data)

    fig = plt.figure(figsize=(12, 12))

    # TIMER DATAFRAME
    timer = pandas.DataFrame(list(data['Timer'].items()), columns=[
                             'Timer', 'Duration'])
    # convert ms to sec
    timer['Duration'] = timer['Duration'].apply(
        lambda x: np.around(x/1000, decimals=2))
    timer['DFA'] = timer['Timer'].apply(
        lambda x: True if 'DFA' in x and x != 'DFA Runtime' else False)
    timer['Timer'] = timer['Timer'].apply(
        lambda x: x[4:] if 'DFA' in x and x != 'DFA Runtime' else x)
    pprint.pprint(timer)

    ax = plt.subplot2grid((2, 2), (0, 0))
    drawTimer(timer.loc[timer['DFA'] == True], ax,
              'Timer', 'Duration', plt, 'Phases')

    ax = plt.subplot2grid((2, 2), (0, 1))
    drawTimer(timer.loc[timer['DFA'] == False], ax,
              'Timer', 'Duration', plt, 'Analysis Steps')

    # COUNTER DATAFRAME
    ax = plt.subplot2grid((2, 1), (1, 0))
    stats_df = pandas.DataFrame(
        list(data['Counter'].items()), columns=['Statistic', 'Count'])
    stats_df['Statistic'] = stats_df['Statistic'].apply(lambda x: x[3:])
    drawCounter(stats_df, ax, 'Statistic', 'Count', plt, 'General Statistics')

    # HISTOGRAM DATAFRAME
    # Gather all histogram data
    # maping: histo type -> {value -> #occurence }
    histo_map = {}
    for prop, values in data.items():
        if "Histogram" in prop:
            histo_map[prop] = values

    # dfacts_df = pandas.DataFrame(list(data['Histogram']['Data-flow facts'].items()), columns=['Value', '#Occurrences'])

    # pprint.pprint(dfacts_df)
    # dfToNumeric(dfacts_df)
    # maxValue = dfacts_df.loc[dfacts_df['Value'].idxmax()]['Value']
    # bins = np.arange(0, maxValue+10, 10)
    # # pprint.pprint(bins)
    # xrange = np.arange(10, maxValue+10, 10)
    # # pprint.pprint(xrange)
    # g = dfacts_df.groupby(pandas.cut(dfacts_df['Value'], bins)).sum()
    # # pprint.pprint(g)
    # g.plot.bar(y='#Occurrences', x='Value', color=['tab:green', 'tab:red'], alpha=0.8, width=1, legend=True, fontsize=9)

    # ax = plt.subplot2grid((3, 3), (2, 1))
    # drawHistogram(g, ax, xrange, '#Occurrences', 'Data-flow facts Dist.')

    plt.tight_layout(pad=0.9, w_pad=0.15, h_pad=1.0)
    plt.show()


if __name__ == "__main__":
    main(sys.argv[1:])
