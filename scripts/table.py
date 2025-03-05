class TableBase(object):
    def __init__(self):
        self.headers = []
        self.rows    = []

    def set_header(self, header):
        assert len(header) > 0
        self.headers = [self.normalize(header)]

    def add_row(self, row):
        assert len(row) > 0
        self.rows.append(self.normalize(row))

    def normalize(self, row):
        tmp = []
        for text, count in self.iter_spans(row):
            assert count >= 1
            if count == 1:
                tmp.append(text)
            else:
                tmp.append((text, count))

        return tmp

    def is_raw(self, row):
        return all(isinstance(val, str) for val in row)

    def iter_spans(self, row):
        for item in row:
            if type(item) is tuple:
                text  = item[0]
                count = item[1]
            else:
                text  = item
                count = 1

            yield (text, count)


class TableValidator(object):
    def __init__(self, table):
        self.table = table
        self.columns = self.get_table_columns_count()
        if self.columns is None:
            raise ValueError("Table doesn't have header which define column layout")
        self.validate()

    def get_table_columns_count(self):
        # only from headers
        for header in self.table.headers:
            if self.table.is_raw(header):
                return len(header)

    def validate(self):
        for i, header in enumerate(self.table.headers):
            n = self.get_columns_count(header)
            if n != self.columns:
                raise ValueError("header #%d has %d column(s), expected %d: %s" % (i, n, self.columns, header))

        for i, row in enumerate(self.table.rows):
            n = self.get_columns_count(row)
            if n != self.columns:
                raise ValueError("row #%d has %d column(s), expected %d: %s" % (i, n, self.columns, row))

    def get_columns_count(self, row):
        n = 0
        for _, count in self.table.iter_spans(row):
            n += count

        return n


ALIGN_RIGHT = '>'
ALIGN_LEFT = '<'
CENTER = '^'


class MarkdownTableRenderer(object):
    def __init__(self, table):
        if len(table.headers) > 1:
            raise ValueError("Markdown does not support multiple header rows")

        self.validator  = TableValidator(table)
        self.table      = table
        self.padding    = 1
        self.widths     = self._calculate_widths()

    def get_headers(self):
        return self.table.headers

    def get_rows(self):
        return self.table.rows

    def _calculate_widths(self):
        width = [0] * self.validator.columns

        # get width from fixed
        for row in self.get_headers() + self.get_rows():
            index = 0
            for text, count in self.table.iter_spans(row):
                w = len(text)
                width[index] = max(w, width[index])
                index += 1

        return width

    def _get_columns_width(self, start, count):
        assert count >= 1

        w = 0
        for index in range(start, start + count):
            w += self.widths[index]
            w += 2 * self.padding

        w += (count - 1)  # for the columns spacing '|'
        return w

    def _render_separator(self, row, fill):
        assert len(fill) == 1

        result = '|'
        index = 0
        for text, count in self.table.iter_spans(row):
            result += ' '
            result += fill * (self._get_columns_width(index, count) - 2)
            result += ' '
            result += '|'
            index  += count

        return result

    def _render_row(self, row, align=None):
        if align is not None:
            def get_align(text):
                return align
        else:
            def get_align(text):
                if is_float_or_int(text):
                    return ALIGN_RIGHT
                else:
                    return ALIGN_LEFT

        result = '|'
        index = 0
        for text, count in self.table.iter_spans(row):
            width   = self._get_columns_width(index, count)
            width  -= 2 * self.padding
            result += ' ' * self.padding
            result += u'{:{align}{width}}'.format(text, align=get_align(text), width=width)
            result += ' ' * self.padding
            result += '|'

            index  += count

        return result

    def get_image(self):
        lines = []

        if self.table.headers:
            header = self.table.headers[0]
            lines.append(self._render_row(header, CENTER))
            lines.append(self._render_separator(header, '-'))

        for row in self.get_rows():
            lines.append(self._render_row(row))

        return '\n'.join(lines)


class Table(TableBase):
    def __unicode__(self):
        renderer = MarkdownTableRenderer(self)
        return renderer.get_image()

    def __str__(self):
        renderer = MarkdownTableRenderer(self)
        return renderer.get_image()


def is_float_or_int(text):
    try:
        float(text)
        return True
    except ValueError:
        return False


if __name__ == '__main__':

    table = Table()

    table.set_header(["procedure", "size", "time"])
    table.add_row(["foo", "100", "0.5"])
    table.add_row(["bar", "105", "1.5"])
    table.add_row(["baz", "111", "0.2"])

    print(table)

    table2 = Table()
    table2.set_header(["size1", "size2", "proc1", "proc2", "proc3"])
    table2.add_row(["1", "2", "a", "b", "c"])
    table2.add_row(["9", "3", "A", "B", "C"])
    table2.add_row(["42", "-", ("N/A", 3)])

    print()
    print(table2)
