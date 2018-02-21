#!/usr/bin/env python3

# author: Philipp D. Schubert

# For DataFrame please refer to https://pandas.pydata.org/pandas-docs/stable/generated/pandas.DataFrame.plot.html
# For colormap please refer to https://matplotlib.org/users/colormaps.html

import json
import pprint
import numpy
import pandas
import sys
import getopt
import matplotlib.pyplot as plt

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
  # pprint.pprint(data)

  fig, axes = plt.subplots(ncols=3, figsize=(18, 8))
  ax = axes[0]

  histogram = pandas.DataFrame(list(data['Set Histogram'].items()), columns=['set size', '# occurrences'])
  # convert all entries into numeric values
  for column in histogram:
  	histogram[column] = pandas.to_numeric(histogram[column])
  histogram = histogram.sort_values('set size', ascending=True)
  histogram.plot(kind='bar', ax=ax, x='set size', y='# occurrences', logy=True, grid=True)
  ax.set_title("Set size histogram")

  ax = axes[1]
  counter = pandas.DataFrame(list(data['Counter'].items()), columns=['counter', '# occurrences'])
  counter.plot(kind='bar', ax=ax, x='counter', y='# occurrences', fontsize=8, logy=True, grid=True)
  ax.set_title("Counter")

  ax = axes[2]
  timer = pandas.DataFrame(list(data['Timer'].items()), columns=['task', 'time'])
  timer.plot(kind='pie', ax=ax, y='time', autopct='%1.1f%%', fontsize=8, labels=timer['task'], colormap='GnBu')
  ax.set_title("Runtime")

  plt.show()

if __name__ == "__main__":
  main(sys.argv[1:])
