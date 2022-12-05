# =============================================================================
#
# ztd.cmake
# Copyright © 2022 JeanHeyd "ThePhD" Meneide and Shepherd's Oasis, LLC
# Contact: opensource@soasis.org
#
# Commercial License Usage
# Licensees holding valid commercial ztd.cmake licenses may use this file in
# accordance with the commercial license agreement provided with the
# Software or, alternatively, in accordance with the terms contained in
# a written agreement between you and Shepherd's Oasis, LLC.
# For licensing terms and conditions see your agreement. For
# further information contact opensource@soasis.org.
#
# Apache License Version 2 Usage
# Alternatively, this file may be used under the terms of Apache License
# Version 2.0 (the "License") for non-commercial use; you may not use this
# file except in compliance with the License. You may obtain a copy of the
# License at
#
#		http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# ============================================================================>

import argparse, json, os, pathlib
from importlib.resources import path

import visualize, sources_parser, benchmarks_parser

import matplotlib, matplotlib.pyplot, matplotlib.ticker
import matplotlib.colors, matplotlib.patches
import matplotlib.container, matplotlib.collections
import numpy, math
import random, bisect

from typing import List, Tuple, Union


def ordinal(value) -> str:
	# https://stackoverflow.com/questions/9647202/ordinal-numbers-replacement
	return '%d%s' % (value, "tsnrhtdd"[(value // 10 % 10 != 1) *
	                                   (value % 10 < 4) * value % 10::4])


def from_unit_scale_comparison(scale: visualize.data_scale):
	return scale.from_unit_scale


def hsv_value_multiply(color: numpy.ndarray, darken_factor: float):
	color_hsv = matplotlib.colors.rgb_to_hsv(color)
	color_hsv[2] = min(max(0, color_hsv[2] * darken_factor), 1)
	return matplotlib.colors.hsv_to_rgb(color_hsv)


def draw_graph(benchmark: visualize.benchmark) -> Tuple[str, any, any, str]:
	# initialize figures and alt text description
	figures, axes = matplotlib.pyplot.subplots(figsize=(12.80, 7.20))
	description = ""

	# get some basic benchmark properties
	benchmark_name = benchmark.analysis_info.name + ' - ' + benchmark.category.name

	# get the values of the time scale to perform bisecting
	benchmark_max: float = benchmark.heuristics.max
	benchmark_min: float = benchmark.heuristics.min
	absolute_range: float = benchmark_max - benchmark_min
	lower_is_better: bool = (
	    benchmark.category.order == visualize.category_order.ascending)

	# some pattern constants, to help us be pretty
	# some color constants, to help us be pretty!
	# and differentiate graphs
	# yapf: disable
	data_point_aesthetics: List[Tuple[str, Union[str, None]]] = [
	 ('#a6cee3', '/'),
	 ('#f255bb', 'O'),
	 ('#00c9ab', '\\'),
	 ('#b15928', 'o'),
	 ('#33a02c', '.'),
	 ('#fb9a99', '*'),
	 ('#e31a1c', '+'),
	 ('#fdbf6f', 'x'),
	 ('#ff7f00', '|'),
	 ('#cab2d6', None),
	 ('#6a3d9a', '-'),
	 ('#ffff99', 'xx'),
	 ('#f5f5f5', '..'),
	 ('#1f78b4', '||'),
	 ('#b2df8a', '**'),
	 ('#cc33cc', '--'),
	 ('#89db59', None),
	 ('#eeeeee', '/o'),
	 ('#555555', '-|'),
	 ('#eeaa00', '*+'),
	 ('#ffff2a', 'x.'),
	 ('#33675c', '+\\'),
	 ('#b3c210', '\\\\'),
	 ('#8c230e', '//')
	]
	#yapf: enable

	# transpose data into forms we need
	num_groups = len(benchmark.groups)
	group_names = [
	    (label.info.name if label.info and label.info.name else label.name)
	    for label in benchmark.groups
	]
	data_label_names = [
	    label.name for label in benchmark.analysis_info.data_labels
	]
	bars: List[Union[matplotlib.pyplot.Text,
	                 matplotlib.container.BarContainer]] = []
	scatters: List[matplotlib.collections.PathCollection] = []
	num_data_labels: int = len(benchmark.analysis_info.data_labels)
	index_num_data_labels: int = num_data_labels - 1
	bar_padding = 0.05
	group_padding = 0.15
	bar_height = 0.50
	group_height = (bar_height * num_data_labels) + (
	    bar_padding * index_num_data_labels) + group_padding
	quarter_bar_height = bar_height * 0.25
	bar_y_positions: List[float] = []

	# Start appending to the description
	# title
	description += "Title: \""
	description += benchmark_name
	description += "\"."
	if benchmark.category.description and len(
	    benchmark.category.description) > 0:
		# optionally-provided description for the category
		description += " Description: \""
		description += benchmark.category.description
		description += "\""
	description += "\n\n"
	description += "There {} {} group{}, and {} data label{} ({}) per each group with data.".format(
	    "is" if num_groups == 1 else "are", num_groups,
	    "" if num_groups == 1 else "s", num_data_labels,
	    "" if num_data_labels == 1 else "s", ", ".join(data_label_names))
	description += " {}.".format(
	    "Lower is better" if lower_is_better else "Higher is better")
	description += " \n"

	# add to description first, here
	# do the actual values of the graph next.
	for group_index, group in enumerate(benchmark.groups):
		ordinal_group_index = (len(benchmark.groups) - group_index -
		                       1) if lower_is_better else group_index
		description += "\n- {} is {}. ".format(
		    group.name, ordinal(ordinal_group_index + 1))
		if group.info and group.info.description and len(
		    group.info.description) > 1:
			description += "Described as: \""
			description += group.info.description
			description += "\"."
		else:
			description += "It has no description."

		description += "\n  "

		err = group.error
		if err != None:
			description += "This group had an error: \""
			description += err
			description += "\"."
		else:
			label: visualize.data_label = group.labels[group.primary_label]
			label_info: visualize.data_label_info = label.info
			statistics: visualize.stats = label.stats
			xscale_bisect_index: int = bisect.bisect_left(
			    label_info.format_list,
			    benchmark_max,
			    key=from_unit_scale_comparison)
			xscale_index = max(xscale_bisect_index - 1, 0)
			xscale: visualize.data_scale = label_info.format_list[
			    xscale_index]
			description += "Measures to a mean of \"{}\" {}, from {} multi-iteration samples.".format(
			    statistics.mean * xscale.to_unit_scale, xscale.name,
			    statistics.data_point_count)

	# draw mean-based bars with error indicators
	# and draw scatter-plot points
	for group_index, group in enumerate(benchmark.groups):
		input_color_index: int = group.info.order_index if (
		    group.info != None
		    and group.info.order_index != None) else group.name_index
		color_index: int = input_color_index % len(data_point_aesthetics)
		aesthetics: Tuple[str,
		                  Union[str,
		                        None]] = data_point_aesthetics[color_index]
		primary_bar_color = aesthetics[0]
		bar_decoration = aesthetics[1]
		# create a sequence of colorings that will keep the related ones grouped together and keep them vaguely the same color but with some differentiation...
		# this might not be the best design for those who are colorblind, however :/
		label_bar_colors: List[numpy.ndarray] = []
		label_edge_colors: List[numpy.ndarray] = []
		for label_index in range(num_data_labels):
			label_bar_color_hsv = matplotlib.colors.rgb_to_hsv(
			    matplotlib.colors.hex2color(primary_bar_color))
			label_bar_color_hsv[2] *= (1.0 - (0.2 * label_index))
			label_bar_color = matplotlib.colors.hsv_to_rgb(
			    label_bar_color_hsv)
			label_edge_color_hsv = matplotlib.colors.rgb_to_hsv(
			    matplotlib.colors.hex2color(primary_bar_color))
			label_edge_color_hsv[2] *= (0.6 - (0.1 * label_index))
			label_edge_color = matplotlib.colors.hsv_to_rgb(
			    label_bar_color_hsv)
			label_bar_colors.append(label_bar_color)
			label_edge_colors.append(label_edge_color)

		err = group.error

		if err != None:
			for label_index in range(num_data_labels):
				bar_y = (group_index *
				         group_height) + (label_index *
				                          (bar_height + bar_padding))
				bar_y_positions.append(bar_y)
				edge_color = hsv_value_multiply(
				    label_edge_colors[label_index], 0.2)
				bars.append(
				    axes.text(absolute_range * 0.02,
				              bar_y + (quarter_bar_height * 2),
				              err,
				              color=edge_color,
				              style='italic',
				              horizontalalignment='left',
				              verticalalignment='center',
				              fontsize='small'))
				continue

		for label_index, label_name in enumerate(group.labels):
			label: visualize.data_label = group.labels[label_name]
			label_info: visualize.data_label_info = label.info
			statistics: visualize.stats = label.stats
			bar_color = label_bar_colors[label_index]
			edge_color = label_edge_colors[label_index]
			bar_y = (group_index *
			         group_height) + (label_index *
			                          (bar_height + bar_padding))
			bar_y_positions.append(bar_y)

			mean = statistics.mean
			stddev = statistics.stddev
			bar: matplotlib.container.BarContainer = axes.barh(
			    bar_y,
			    mean,
			    height=bar_height,
			    xerr=stddev,
			    linewidth=0.2,
			    edgecolor=edge_color,
			    color=bar_color,
			    hatch=bar_decoration,
			    align='edge',
			    error_kw={
			        "capsize": 5.0,
			        "mew": 1.2,
			        "ecolor": 'black',
			    },
			    alpha=0.82)
			bars.append(bar)
			# the scatter plot should be semi-transparent in color...
			xscatter = label.data
			xscatter_len = len(xscatter)
			yscatter = [
			    bar_y + random.uniform(quarter_bar_height,
			                           bar_height - quarter_bar_height)
			    for _ in xscatter
			]
			scatter_alpha = 0.20 if xscatter_len < 11 else 0.10 if xscatter_len < 101 else 0.05 if xscatter_len < 1001 else 0.002
			scatter: matplotlib.collections.PathCollection = axes.scatter(
			    xscatter,
			    yscatter,
			    color=bar_color,
			    edgecolor='#000000',
			    linewidth=0.5,
			    alpha=scatter_alpha)
			scatters.append(scatter)

	xscale_bisect_index: int = bisect.bisect_left(
	    label_info.format_list, benchmark_max, key=from_unit_scale_comparison)
	xscale_index = max(xscale_bisect_index - 1, 0)
	xscale: visualize.data_scale = label_info.format_list[xscale_index]

	def time_axis_formatting(value: float, _: int):
		if value == 0:
			return '0'
		if value.is_integer():
			return '{0:.0f}'.format(value * xscale.to_unit_scale)
		return '{0:.1f}'.format(value * xscale.to_unit_scale)

	xlimit: float = benchmark_max + (absolute_range * 0.15)
	adjusted_xlimit = xlimit * xscale.to_unit_scale
	adjusted_xlimit = round(adjusted_xlimit)
	adjusted_xlimit *= xscale.from_unit_scale
	axes.set_xlim(left=0, right=xlimit)
	axes.set_xticks(numpy.arange(0, xlimit, xlimit / 10.0))
	axes.xaxis.set_major_formatter(
	    matplotlib.ticker.FuncFormatter(time_axis_formatting))

	# have ticks drawn from base of bar graph
	# to text labels
	y_tick_height = group_height
	y_ticks = [((y_index + 0.45) * y_tick_height)
	           for y_index, _ in enumerate(benchmark.groups)]
	y_limits = [
	    bar_y_positions[0] - group_padding,
	    bar_y_positions[-1] + bar_height + group_padding
	]

	# set the tick spacing
	axes.set_yticks(y_ticks)
	# label each group (each cluster along the y axes)
	# with the names of the benchmarks we ran
	axes.set_yticklabels(group_names)
	# set the visual limits so we have good spacing
	axes.set_ylim(y_limits)

	# if we have 2 or more data points,
	# we have to say what each graph part means
	is_better_text = 'lower is better' if lower_is_better else 'higher is better'
	data_label_descriptions: List[str] = [
	    ordinal(label_index + 1) + " is " + label_info.name for label_index,
	    label_info in enumerate(benchmark.analysis_info.data_labels[::-1])
	]
	axes.set_xlabel('measured in ' + xscale.name + ' - ' + is_better_text)
	axes.set_ylabel(',\n'.join(data_label_descriptions))

	# create the benchmark name
	axes.set_title(benchmark_name)
	# get a nice, clean layout
	figures.tight_layout()

	# make sure to adjust top and bottoms
	figures.subplots_adjust(bottom=0.1)

	description += "\n"

	return benchmark_name, figures, axes, description


def draw_graphs(output_dir: str, benchmarks: List[visualize.benchmark]):
	for benchmark in benchmarks:
		file_name, figures, axes, description = draw_graph(benchmark)
		# save drawn figures
		output_file = os.path.join(output_dir, file_name)
		output_file += ".png"
		description_file_name = file_name + ".txt"
		output_description_file = output_file + ".txt"
		print("Saving graph: {} (to '{}')".format(file_name, output_file))
		parent_path = pathlib.Path(output_file).parent
		os.makedirs(parent_path, exist_ok=True)
		matplotlib.pyplot.savefig(output_file, format="png")
		matplotlib.pyplot.close(figures)
		print("Saving graph description: {} (to '{}')".format(
		    description_file_name, output_description_file))
		description_file = open(output_description_file, "wb")
		description_file.write(description.encode('utf-8'))
		description_file.close()


def main():
	# make sure we have dependable jitter for graphs
	random.seed(1782905257495843795)

	parser = argparse.ArgumentParser(
	    description=
	    'Generate graphs from a json listing of data in various formats...')
	parser.add_argument('-c',
	                    '--config',
	                    nargs='?',
	                    default=['graph_config.json'],
	                    type=argparse.FileType('r'))
	parser.add_argument('-i',
	                    '--inputs',
	                    nargs='*',
	                    default=['benchmarks.json'])
	parser.add_argument('-o', '--output_dir', nargs='?')

	args = parser.parse_args()

	config_name: str = ""
	if not args.output_dir:
		if args.config and args.config.name:
			config_name: str = args.config.name
			directoryname, filename = os.path.split(config_name)
			args.output_dir = directoryname
		else:
			args.output_dir = os.getcwd()

	config_json: dict = json.load(args.config)
	info: visualize.analysis_info = sources_parser.parse_sources_from_json(
	    config_json, config_name, args.inputs)
	benchmarks: List[
	    visualize.
	    benchmark] = benchmarks_parser.parse_benchmarks_from_sources(info)
	draw_graphs(args.output_dir, benchmarks)


if __name__ == '__main__':
	main()
