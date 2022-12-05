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

import enum, statistics, re
from typing import List, Dict, Optional


def is_noop_category(n: str):
	noop_names = ["noop", "no-op", "no_op", "no op"]
	s = n.casefold()
	return s in noop_names


class graph_scaling(enum.Enum):
	absolute = 0
	relative = 1


class category_order(enum.Enum):
	ascending = 0
	descending = 1


class data_label_format(enum.Enum):
	custom = -1
	clock = 1


class scaling_info():

	def __init__(self, scaling: graph_scaling, to: str = "") -> None:
		self.type = scaling
		self.to = to


class category_info():

	def __init__(self,
	             name: str,
	             scale: scaling_info,
	             order: category_order,
	             pattern: Optional[str] = None,
	             exclude_pattern: Optional[str] = None,
	             description: str = None) -> None:
		self.name: str = name
		self.pattern: re.Pattern[str] = re.compile(
		    pattern) if pattern else re.compile(re.escape(name) + "_")
		self.exclude_pattern = re.compile(
		    exclude_pattern) if exclude_pattern else re.compile("x^")
		self.scale: scaling_info = scale
		self.order: category_order = order
		self.description: str = description if isinstance(description,
		                                                  str) else ""


always_included_category = category_info(
    "ztd.tools.always_included", scaling_info(graph_scaling.relative),
    category_order.ascending, "", "",
    "A universal catch-all category for data groups and names that are always included in all other categories."
)


class data_scale():

	def __init__(self, abbreviation: str, name: str, from_unit_scale: float,
	             to_unit_scale: float) -> None:
		self.abbreviation = abbreviation
		self.name = name
		self.to_unit_scale = to_unit_scale
		self.from_unit_scale = from_unit_scale


class data_label_info():
	clock_time_scales: List[data_scale] = [
	    data_scale("fs", "femtoseconds", 1e-15, 1e+15),
	    data_scale("ps", "picoseconds", 1e-12, 1e+12),
	    data_scale("ns", "nanoseconds", 1e-9, 1e+9),
	    data_scale("µs", "microseconds", .000001, 1000000),
	    data_scale("us", "microseconds", .000001, 1000000),
	    data_scale("ms", "milliseconds", .001, 100),
	    data_scale("s", "seconds", 1, 1),
	    data_scale("m", "minutes", 60, 1 / 60),
	    data_scale("h", "hours", 60 * 60, (1 / 60) / 60),
	]

	unknown_time_scales: List[data_scale] = [data_scale("", "", 1, 1)]

	def __init__(self,
	             id: str = "real_time",
	             name: str = None,
	             format: data_label_format = data_label_format.clock,
	             primary: bool = False,
	             description: str = "") -> None:
		self.id: str = id
		self.name: str = self.id if name is None else name
		self.format: data_label_format = format
		self.format_list: List[
		    data_scale] = data_label_info.clock_time_scales if self.format == data_label_format.clock else data_label_info.unknown_time_scales
		self.primary: bool = primary
		self.description: str = description

	def get_scale_by_abbreviation(self, abbreviation_name: str) -> data_scale:
		for scale in self.format_list:
			if scale.abbreviation == abbreviation_name:
				return scale
		return data_label_info.unknown_time_scales[0]


class data_group_info():

	def __init__(self,
	             name: str,
	             order_index: Optional[int],
	             pattern: str = None,
	             description: str = "",
	             always_included: bool = False):
		self.name = name
		self.order_index: Optional[int] = order_index
		self.pattern: re.Pattern[str] = re.compile(
		    pattern) if pattern else re.compile(re.escape(name))
		self.description: str = description if description else ""
		self.always_included: bool = always_included


class stats():

	def __init__(self, data: List[float]) -> None:
		self.data_point_count = len(data)
		if self.data_point_count < 1:
			self.min = 0.0
			self.max = 0.0
			self.stddev = 0.0
			self.mean = 0.0
			self.median = 0.0
			self.mode = 0.0
			self.index_of_dispersion = 0.0
		else:
			self.min = min(data)
			self.max = max(data)
			self.mean = statistics.mean(data)
			self.stddev = statistics.stdev(data, self.mean)
			self.median = statistics.median(data)
			self.mode = statistics.mode(data)
			self.index_of_dispersion = 0
			if (self.mean != 0):
				self.index_of_dispersion = statistics.variance(
				    data) / self.mean


class source_info():

	def __init__(self,
	             file_name: str,
	             prefix: str = "",
	             required: bool = False) -> None:
		self.file_name: str = file_name
		self.prefix: str = prefix
		self.required: bool = required


class analysis_info():

	def __init__(self):
		self.name = ""
		self.default_scale: scaling_info = scaling_info(
		    graph_scaling.relative, "base")
		self.categories: List[category_info] = []
		self.data_labels: List[data_label_info] = []
		self.data_groups: List[data_group_info] = []
		self.sources: List[source_info] = []
		self.prefixes_to_remove: List[str] = []
		self.suffixes_to_remove: List[str] = []
		self.at_least_one_required: bool = True


class data_label():

	def __init__(self, label: str, category: category_info,
	             info: data_label_info, data: List[float]) -> None:
		self.label = label
		self.category: category_info = category
		self.info: data_label_info = info
		self.is_noop_category = is_noop_category(info.id)
		self.data = data
		self.stats = stats(self.data)
		self.error: Optional[str] = None


class data_group():

	def __init__(self,
	             name: str,
	             category: category_info,
	             labels: Dict[str, data_label],
	             primary_label: str,
	             info: data_group_info = None) -> None:
		self.name: str = name
		self.info: data_group_info = info
		self.category: category_info = category
		self.always_included: bool = self.info.always_included if self.info else False
		self.labels: Dict[str, data_label] = labels
		self.primary_label: str = primary_label
		maybe_err = [
		    self.labels[name].error
		    for name in self.labels
		    if self.labels[name].error is not None
		]
		self.name_index: int = 0
		self.error: Optional[str] = maybe_err[0] if len(
		    maybe_err) > 0 else None


class benchmark():

	def __init__(self, category: category_info, analysis: analysis_info,
	             groups: List[data_group], heuristics: stats) -> None:
		self.analysis_info: analysis_info = analysis
		self.category: category_info = category
		self.groups: List[data_group] = groups
		self.heuristics: stats = heuristics
