#!/usr/bin/env python3
import json
import os
import tkinter as tk
from tkinter import filedialog as tkfile
from typing import List, Dict

import matplotlib.pyplot as plt
import pandas as pd
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg, NavigationToolbar2Tk


def write_text_and_disable(widget_txt: tk.Text, txt: str):
    widget_txt.configure(state=tk.NORMAL)
    widget_txt.delete('1.0', tk.END)
    widget_txt.insert(tk.END, txt)
    widget_txt.configure(state=tk.DISABLED)


def make_horizontal_entry_with_label(parent, label, variable):
    frame = tk.Frame(parent)
    frame.pack(side=tk.TOP, anchor=tk.W, expand=True, fill=tk.X, padx=10)
    tk.Label(frame, text=label).pack(side=tk.LEFT)
    tk.Entry(frame, width=40, textvariable=variable).pack(side=tk.LEFT, fill=tk.X, expand=True)


def parse_string(entry_widget: tk.Entry, default_value: str):
    try:
        value = str(entry_widget.get())
    except ValueError:
        value = default_value
        print("Scale '{:s}' could not be parsed to string.".format(entry_widget.get()))
    return value


def parse_float(entry_widget: tk.Entry, default_value: float):
    try:
        value = float(entry_widget.get())
    except ValueError:
        value = default_value
        print("Scale '{:s}' could not be parsed to float.".format(entry_widget.get()))
    return value


class HistoryRecord:
    """ Single record of a plot-line containing references to the experiment-data, columns and scales.
    """
    def __init__(self):
        self.experiment_name = None  # name of the experiment
        self.platform_name = None  # name of the platform
        self.data_file_name = None  # name of the source .csv file containing data

        self.x_col_name = None  # name of the column used for x-axis
        self.x_scale = 1  # scale of the x-axis
        self.y_col_name = None  # name of the column used for y-axis
        self.y_scale = 1  # scale of the y-axis
        self.y_label = None  # label (legend) for the y-axis data

        self.y_subtract = None  # column to be subtracted from y_col_name data
        self.y_subtract_scale = 1  # scale of the subtracted column

        self.smoothing_window_size = None  # size of the smoothing window

    def set_data_paths(self, experiment_name: str, platform_name: str, data_file_name: str):
        assert isinstance(experiment_name, str), "experiment_name must be a string."
        assert len(experiment_name) > 0, "experiment_name must not be empty."
        assert isinstance(platform_name, str), "platform_name must be a string."
        assert len(platform_name) > 0, "platform_name must not be empty."
        assert isinstance(data_file_name, str), "data_file_name must be a string."
        assert len(data_file_name) > 0, "data_file_name must not be empty."
        assert data_file_name.endswith(".csv"), "data_file_name must end with '.csv'."

        self.experiment_name = experiment_name
        self.platform_name = platform_name
        self.data_file_name = data_file_name

    def set_x_data(self, x_col_name: str, x_scale: float):
        assert isinstance(x_col_name, str), "x_col_name must be a string."
        assert len(x_col_name) > 0, "x_col_name must not be empty."
        assert isinstance(x_scale, (float, int)), "x_scale must be a number."

        self.x_col_name = x_col_name
        self.x_scale = x_scale

    def set_y_data(self, y_col_name: str, y_scale: float, y_label: str):
        assert isinstance(y_col_name, str), "y_col_name must be a string."
        assert len(y_col_name) > 0, "y_col_name must not be empty."
        assert isinstance(y_scale, (float, int)), "y_scale must be a number."
        assert isinstance(y_label, str), "y_label must be a string."

        self.y_col_name = y_col_name
        self.y_scale = y_scale
        self.y_label = y_label

    def set_scale_data(self, y_subtract: str, y_subtract_scale: float):
        assert isinstance(y_subtract, str), "y_subtract must be a string."
        assert isinstance(y_subtract_scale, (float, int)), "y_subtract_scale must be a number."

        self.y_subtract = y_subtract
        self.y_subtract_scale = y_subtract_scale

    def set_smoothing_window_size(self, size: int):
        self.smoothing_window_size = size

    def to_dict(self) -> dict:
        return self.__dict__

    def from_dict(self, dct: dict):
        self.experiment_name = dct["experiment_name"]
        self.platform_name = dct["platform_name"]
        self.data_file_name = dct["data_file_name"]
        self.x_col_name = dct["x_col_name"]
        self.x_scale = dct["x_scale"]
        self.y_col_name = dct["y_col_name"]
        self.y_scale = dct["y_scale"]
        self.y_label = dct["y_label"]
        self.y_subtract = dct["y_subtract"]
        self.y_subtract_scale = dct["y_subtract_scale"]
        self.smoothing_window_size = dct["smoothing_window_size"]

    def get_path(self, experiments_root: str) -> str:
        """:return path to the experiment file or empty string if such does not exists"""
        if not self.experiment_name or not self.platform_name or not self.data_file_name:
            return ""
        path = os.path.join(experiments_root, self.experiment_name, "data", self.platform_name, self.data_file_name)
        if os.path.exists(path):
            return path
        else:
            print("Path '{:s}' does not exist". format(path))
            return ""

    def use_smoothing(self) -> bool:
        return True if self.smoothing_window_size else False

    def use_subtract(self) -> bool:
        return self.y_subtract is not None


class PlotHistory:
    def __init__(self):
        self.list_of_records = []  # List containing List[HistoryRecord] elements
        self.x_axis_title = ""  # title of the x-axis
        self.y_axis_title = ""  # title of the y-axis
        self.figure_title = ""  # title of the figure

    def clear(self):
        self.list_of_records = []
        self.x_axis_title = ""
        self.y_axis_title = ""
        self.figure_title = ""

    def remove_last(self):
        self.list_of_records = self.list_of_records[:-1]

    def add_records(self, records: List[HistoryRecord]):
        self.list_of_records.append(records)

    def set_x_title(self, title: str):
        self.x_axis_title = title

    def set_y_title(self, title: str):
        self.y_axis_title = title

    def set_figure_title(self, title: str):
        self.figure_title = title

    def to_dict(self) -> dict:
        return {"x_axis_title": self.x_axis_title,
                "y_axis_title": self.y_axis_title,
                "figure_title": self.figure_title,
                "list_of_records": [[rec.to_dict() for rec in records] for records in self.list_of_records]}

    def save_to_file(self, outfile):
        json.dump(self.to_dict(), outfile, indent=2)

    def load_from_file(self, file_path):
        data = json.load(file_path)
        self.x_axis_title = data["x_axis_title"]
        self.y_axis_title = data["y_axis_title"]
        self.figure_title = data["figure_title"]
        self.list_of_records = []

        for records in data["list_of_records"]:
            lst = []
            for dct in records:
                rec = HistoryRecord()
                rec.from_dict(dct)
                lst.append(rec)
            self.list_of_records.append(lst)

    def history_not_empty(self) -> bool:
        return len(self.list_of_records) > 0


class ScrollFrame(tk.Frame):
    """
    Class adding a scrollbar to a frame, taken from https://gist.github.com/mp035/9f2027c3ef9172264532fcd6262f3b01
    """
    def __init__(self, parent, *args, **kwargs):
        super().__init__(parent, *args, **kwargs)  # create a frame (self)

        self.canvas = tk.Canvas(self, borderwidth=0, highlightthickness=0)  # place canvas on self
        self.view_port = tk.Frame(self.canvas)  # place a frame on the canvas, this frame will hold the child widgets
        self.vsb = tk.Scrollbar(self, orient="vertical", command=self.canvas.yview)  # place a scrollbar on self
        self.canvas.configure(yscrollcommand=self.vsb.set)  # attach scrollbar action to scroll of canvas

        self.vsb.pack(side="right", fill="y")  # pack scrollbar to right of self
        self.canvas.pack(side="left", fill="both", expand=True)  # pack canvas to left of self and expand to fil
        self.canvas_window = self.canvas.create_window((4, 4), window=self.view_port, anchor="nw",
                                                       # add view port frame to canvas
                                                       tags="self.view_port")

        self.view_port.bind("<Configure>", self.on_frame_configure)  # event whenever the size of the viewPort changes
        self.canvas.bind("<Configure>", self.on_canvas_configure)  # event whenever the size of the viewPort changes

        # perform an initial stretch on render, otherwise the scroll region has a tiny border until the first resize
        self.on_frame_configure(None)

    def on_frame_configure(self, event):
        '''Reset the scroll region to encompass the inner frame (whenever the size of the frame changes)'''
        self.canvas.configure(scrollregion=self.canvas.bbox("all"))

    def on_canvas_configure(self, event):
        '''Reset the canvas window to encompass inner frame when required'''
        canvas_width = event.width
        self.canvas.itemconfig(self.canvas_window, width=canvas_width)


class FigureFrame(tk.Frame):
    """Frame containing a (matplotlib) figure and the toolbar for plotting graphs."""
    def __init__(self, parent, *args, **kwargs):
        super().__init__(parent, *args, **kwargs)  # create a frame (self)

        self.figure = plt.Figure(figsize=(8, 6), dpi=100)  # figure for the plotting
        self.axis = self.figure.add_subplot(111)
        self.toolbar = None

        # Shrink current axis by 20% (create space for legend)
        box = self.axis.get_position()
        self.axis.set_position([box.x0, box.y0, box.width * 0.8, box.height])

        self.make_widgets()

    def make_widgets(self):
        """Create the canvas and toolbar widgets"""
        canvas = FigureCanvasTkAgg(self.figure, master=self)
        canvas.draw()

        self.toolbar = NavigationToolbar2Tk(canvas, self)
        self.update_toolbar()
        canvas.get_tk_widget().pack(side=tk.TOP, fill=tk.BOTH, expand=1)

    def update_toolbar(self):
        self.toolbar.update()

    def clear_axis(self):
        self.axis.clear()

    def plot(self, data_x, data_y, label):
        self.axis.plot(data_x, data_y, label=label)

    def set_x_axis_label(self, label):
        self.axis.set_xlabel(label)

    def set_y_axis_label(self, label):
        self.axis.set_ylabel(label)

    def set_title(self, title):
        self.axis.set_title(title)

    def update_figure(self):
        """Update the canvas and toolbar history and create the legend"""
        handles, labels = self.axis.get_legend_handles_labels()
        if labels:
            self.axis.legend(loc='center left', bbox_to_anchor=(1, 0.5))
        self.axis.autoscale(tight=False)
        self.figure.canvas.draw()
        self.figure.canvas.flush_events()
        self.update_toolbar()


class FrameOpenFile(tk.Frame):
    def __init__(self, parent, update_plotting_selector_method, *args, **kwargs):
        super().__init__(parent, *args, **kwargs)  # create a frame (self)

        self.update_plotting_selector_method = update_plotting_selector_method
        self.csv_path = tk.StringVar(value="^ Select a .csv file. ^")  # path to the selected .csv file
        self.txt_open = None  # text area showing the selected path
        self.df = None  # Pandas dataframe loaded from the selected path
        self.df_cols = []  # columns of the dataframe

        self.make_widgets()

    def make_widgets(self):
        # - open button
        tk.Button(self, command=self.open_csv, text="Open") \
            .pack(side=tk.TOP, fill=tk.X, expand=True, anchor=tk.NW, padx=10)

        # - selected file text
        self.txt_open = tk.Text(self, width=50, height=4, wrap=tk.CHAR, spacing3=1)
        write_text_and_disable(self.txt_open, "Please, select a .csv file by clicking on the <Open> button above.")
        self.txt_open.pack(side=tk.TOP, anchor=tk.W, expand=True, padx=10, fill=tk.X)

    def open_csv(self):
        file_name = tkfile.askopenfilename(filetypes=[("CSV files", "*.csv")])
        #file_name = "/home/benedond/Doktorat/Vyzkum/Projects/thermac-experiments-and-visualizations/experiments/assign-fp64div/data/imx8/core1.csv"
        if file_name:
            self.csv_path.set(file_name)
            write_text_and_disable(self.txt_open, file_name)
            if os.path.exists(file_name):  # Read csv file
                self.df = pd.read_csv(file_name, comment="#", index_col=0)
                #self.df = pd.DataFrame(data={'col1' : [1, 2], 'col2': [3, 4], 'col3': [3, 4], 'col4': [3, 4]})
                self.df_cols = sorted(list(self.df.columns.values))
                self.update_plotting_selector_method(self.df_cols)

    def get_df(self) -> pd.DataFrame:
        return self.df

    def get_df_cols(self) -> List[str]:
        return self.df_cols

    def get_csv_path(self) -> str:
        return self.csv_path.get()

    def clear_selection(self):
        self.csv_path.set("^ Select a .csv file. ^")
        write_text_and_disable(self.txt_open, "^ Select a .csv file. ^")
        self.df = None
        self.df_cols = []


class FramePlottingSelector(tk.LabelFrame):
    def __init__(self, parent, *args, **kwargs):
        super().__init__(parent, *args, **kwargs)  # create a frame (self)
        self.configure(text="Plotting selection:")
        self.x_axis_control = None  # placeholder for variable deciding x-axis data (radiobutton)
        self.y_axis_chck = []  # list of checkbuttons for plotting data (df column) selection
        self.y_axis_labels = dict()  # user-defined labels of the data
        self.y_axis_scale = dict()  # user-defined scaling of the data
        self.df_cols = []
        
        self.subtract = tk.BooleanVar(value=0)  # true if some column should be subtracted
        self.subtract_col = tk.StringVar()  # selection of the column to be subtracted
        self.subtract_scale = None  # scaling of the subtract column

    def clear_options(self):
        # Destroy the previous options
        for widget in self.winfo_children():
            widget.destroy()

    def update_plotting_selector(self, df_cols):
        self.df_cols = df_cols
        self.y_axis_chck = []
        self.y_axis_scale = dict()
        self.y_axis_labels = dict()

        # Destroy the previous options
        self.clear_options()

        # Create new options
        if len(df_cols) > 0:
            row_id = 0
            tk.Label(self, text="x", padx=5).grid(row=row_id, column=0, stick=tk.W)
            tk.Label(self, text="y", padx=5).grid(row=row_id, column=1, stick=tk.W)
            tk.Label(self, text="label").grid(row=row_id, column=2, stick=tk.W)
            tk.Label(self, text="scale").grid(row=row_id, column=3, stick=tk.W)
            row_id += 1

            x_axis_default = "time/ms" if "time/ms" in df_cols else df_cols[0]
            self.x_axis_control = tk.StringVar(value=x_axis_default)

            for col in df_cols:
                var = tk.StringVar()

                tk.Radiobutton(self, variable=self.x_axis_control, value=col).grid(row=row_id, column=0, stick=tk.W)
                tk.Checkbutton(self, text=col, variable=var, onvalue=col, offvalue="").grid(row=row_id, column=1, stick=tk.W, padx=1)
                self.y_axis_chck.append(var)

                lbl = tk.Entry(self, width=20, justify=tk.LEFT)
                lbl.grid(row=row_id, column=2, padx=1, stick=tk.EW)
                lbl.insert(tk.END, col)
                self.y_axis_labels[col] = lbl

                scl = tk.Entry(self, width=8)
                scl.insert(tk.END, "1")
                scl.grid(row=row_id, column=3, padx=1, stick=tk.EW)
                self.y_axis_scale[col] = scl

                row_id += 1


            # subtract menu
            tk.Checkbutton(self, text="subtract:", variable=self.subtract).grid(row=row_id, column=1, pady=10, stick=tk.W)
            tk.OptionMenu(self, self.subtract_col, *self.df_cols).grid(row=row_id, column=2, pady=10, stick=tk.W)
            self.subtract_scale = tk.Entry(self, width=8)
            self.subtract_scale.insert(tk.END, "1")
            self.subtract_scale.grid(row=row_id, column=3, stick=tk.EW, padx=1)

            self.columnconfigure(0, weight=0)
            self.columnconfigure(1, weight=0)
            self.columnconfigure(2, weight=1)
            self.columnconfigure(3, weight=1)

    def get_x_column_name(self) -> str:
        return self.x_axis_control.get()

    def get_y_column_names_selected(self) -> List[str]:
        return list([chck.get() for chck in self.y_axis_chck if chck.get()])

    def get_y_column_scales(self) -> Dict[str, float]:
        return {col: parse_float(self.y_axis_scale[col], 1) for col in self.df_cols}

    def get_y_column_labels(self) -> Dict[str,str]:
        return {col: parse_string(self.y_axis_labels[col], "") for col in self.df_cols}

    def get_subtract(self) -> bool:
        return self.subtract.get()

    def get_subtract_column(self) -> str:
        return self.subtract_col.get()

    def get_subtract_column_scale(self) -> float:
        return parse_float(self.subtract_scale, 1)


class FramePlottingOptions(tk.LabelFrame):
    def __init__(self, parent, *args, **kwargs):
        super().__init__(parent, *args, **kwargs)  # create a frame (self)
        self.configure(text="Options:")
        self.smoothing = tk.BooleanVar(value=False)  # use smoothing (rolling window mean) for plotting the data
        self.smoothing_window = tk.IntVar(value=10)  # size of the smoothing window
        self.clear = tk.BooleanVar(value=False)  # clear the figure before the next plot
        self.x_desc = tk.StringVar(value="")  # title of the x-axis
        self.y_desc = tk.StringVar(value="")  # title of the y-axis
        self.fig_desc = tk.StringVar(value="")  # title of the figure

        self.make_widgets()

    def make_widgets(self):
        frame_check_buttons = tk.Frame(self)  # Frame for horizontal alignment of check buttons
        frame_check_buttons.pack(side=tk.TOP, anchor=tk.NW, expand=True, fill=tk.X)

        # - clear
        chck_clear = tk.Checkbutton(frame_check_buttons, text="clear before plotting", variable=self.clear)
        chck_clear.pack(side=tk.LEFT, anchor=tk.W, expand=False, padx=10)

        # - use smoothing
        chck_smoothing = tk.Checkbutton(frame_check_buttons, text="use smoothing", variable=self.smoothing)
        chck_smoothing.pack(side=tk.LEFT, anchor=tk.W, expand=False, padx=10)

        # - smoothing width (slider)
        frame_scale = tk.Frame(self)
        frame_scale.pack(side=tk.TOP, anchor=tk.NW, expand=True, fill=tk.X, pady=5, padx=10)
        tk.Label(frame_scale, text="Smoothing window size:").pack(side=tk.LEFT)
        tk.Scale(frame_scale, orient=tk.HORIZONTAL, variable=self.smoothing_window, from_=1, to=100).pack(side=tk.LEFT,
                                                                                                          fill=tk.X,
                                                                                                          expand=True)
        # Plot descriptions
        make_horizontal_entry_with_label(self, "X-axis title:", self.x_desc)
        make_horizontal_entry_with_label(self, "Y-axis title:", self.y_desc)
        make_horizontal_entry_with_label(self, "Figure title:", self.fig_desc)

    def get_clear(self) -> bool:
        """:return the state of the <clear> checkbox"""
        return self.clear.get()

    def get_smoothing(self) -> bool:
        """:return the state of the <smoothing> checkbox"""
        return self.smoothing.get()

    def get_smoothing_window_size(self) -> int:
        return self.smoothing_window.get()

    def get_x_axis_label(self) -> str:
        return self.x_desc.get()

    def get_y_axis_label(self) -> str:
        return self.y_desc.get()

    def get_figure_title(self) -> str:
        return self.fig_desc.get()

    def set_x_axis_label(self, val: str):
        self.x_desc.set(val)

    def set_y_axis_label(self, val: str):
        self.y_desc.set(val)

    def set_figure_title(self, val: str):
        self.fig_desc.set(val)


class FrameExponentialFitting(tk.Frame):
    def __init__(self, parent, *args, **kwargs):
        super().__init__(parent, *args, **kwargs)  # create a frame (self)


class ThermacVisualizer(tk.Frame):
    def __init__(self, parent, experiments_root_path: str, *args, **kwargs):
        tk.Frame.__init__(self, parent, *args, **kwargs)
        if experiments_root_path is not None:
            assert os.path.exists(experiments_root_path), "Root path to the experiments folder must be specified."
            assert len(experiments_root_path) > 0, "Root path to the experiments folder must not be empty."
        else:
            print("Path to the experiments root was not specified. Templating functions will be disabled.")
        self.parent = parent
        self.experiments_root_path = experiments_root_path
        self.configure(background='white')
        self.winfo_toplevel().title("Thermac Data Visualizer")

        self.plot_history = PlotHistory()

        # Create the major frames
        # - frame to spawn the menu from top to bottom
        frame_options_container = ScrollFrame(self, bd=1, relief=tk.RAISED)
        frame_options_container.grid(row=0, column=0, stick="nsew", columnspan=1, rowspan=1)

        # - frame containing menu
        self.frame_menu = tk.Frame(frame_options_container.view_port)  # "menu" frame
        self.frame_menu.pack(side=tk.TOP, fill=tk.BOTH, expand=False)

        # -- open file
        self.frame_open_file = FrameOpenFile(self.frame_menu, self.update_plotting_selector)
        self.frame_open_file.pack(side=tk.TOP, fill=tk.X, anchor=tk.NW, expand=True, padx=5, pady=2)

        # -- plotting selector
        self.frame_plot_selector = FramePlottingSelector(self.frame_menu)
        self.frame_plot_selector.pack(side=tk.TOP, fill=tk.X, anchor=tk.NW, expand=True,  padx=5, pady=2)

        # -- plot options
        self.frame_plot_options = FramePlottingOptions(self.frame_menu)
        self.frame_plot_options.pack(side=tk.TOP, fill=tk.X, anchor=tk.NW, expand=True, padx=5, pady=2)

        # - frame with the figure
        self.frame_figure = FigureFrame(self)
        self.frame_figure.grid(row=0, column=1, stick="nsew")

        self.make_widgets()

        # Configure the grid
        self.grid_rowconfigure(0, weight=1)
        self.grid_columnconfigure(0, minsize=400, weight=5)
        self.grid_columnconfigure(1, minsize=400, weight=15)

    def make_widgets(self):
        # - save history button
        frame_plot_history = tk.LabelFrame(self.frame_menu, text="Plotting:")
        frame_plot_history.pack(side=tk.TOP, fill=tk.X, anchor=tk.NW, expand=True, padx=5, pady=5)

        # - plotting button
        btn_plot = tk.Button(frame_plot_history, text="Plot", command=self.plot)
        btn_plot.grid(row=0, column=0, columnspan=3, sticky=tk.NSEW, pady=5)

        if self.experiments_root_path is not None:
            tk.Button(frame_plot_history, text="Back", command=self.history_step_back).grid(row=1, column=0, sticky=tk.NSEW, pady=5)
            tk.Button(frame_plot_history, text="Save as template", command=self.save_history_to_file).grid(row=1, column=1, sticky=tk.NSEW, pady=5)
            tk.Button(frame_plot_history, text="Load template", command=self.load_history_from_file).grid(row=1, column=2, sticky=tk.NSEW, pady=5)

            frame_plot_history.grid_columnconfigure(0, weight=1)
            frame_plot_history.grid_columnconfigure(1, weight=1)
            frame_plot_history.grid_columnconfigure(2, weight=1)
        else:
            frame_plot_history.grid_columnconfigure(0, weight=1)

    def update_plotting_selector(self, cols):
        """Update the plotting selector frame"""
        self.frame_plot_selector.update_plotting_selector(cols)
        #self.frame_plot_selector.update_idletasks()

    def get_clear(self) -> bool:
        """Finds whether the plotting area should be cleared before the next plot.
        :return True if the figure should be cleared before the next plot"""
        return self.frame_plot_options.get_clear()

    def clear_figure(self):
        """Clear axis of the figure"""
        self.frame_figure.clear_axis()

    def clear(self):
        """Clear axis of the figure and the plot history."""
        self.clear_figure()
        self.plot_history.clear()

    def get_df(self) -> pd.DataFrame:
        """:return the current dataframe"""
        return self.frame_open_file.get_df()

    def get_df_cols(self) -> List[str]:
        """:return list of the df column names"""
        return self.frame_open_file.get_df_cols()

    def get_x_column_name(self) -> str:
        """:return the label (name of the column in the df) of the data used for x-axis"""
        return self.frame_plot_selector.get_x_column_name()

    def get_y_column_names_selected(self) -> List[str]:
        """:return list of the selected df column names used for y-axis plot"""
        return self.frame_plot_selector.get_y_column_names_selected()

    def get_y_column_scales(self) -> Dict[str,float]:
        """:return dcitionary containing column_name -> scaling factor mapping"""
        return self.frame_plot_selector.get_y_column_scales()

    def get_y_column_labels(self) -> Dict[str,str]:
        """:return dcitionary containing column_name -> scaling factor mapping"""
        return self.frame_plot_selector.get_y_column_labels()

    def get_subtract(self) -> bool:
        """:return true if column should be subtracted"""
        return self.frame_plot_selector.get_subtract()

    def get_subtract_column(self) -> str:
        """:return name of the column to be subtracted"""
        return self.frame_plot_selector.get_subtract_column()

    def get_subtract_column_scale(self) -> float:
        """:return scaling factor of the subtracted column"""
        return self.frame_plot_selector.get_subtract_column_scale()

    def get_smoothing(self) -> bool:
        """:return True if smoothing should be activated"""
        return self.frame_plot_options.get_smoothing()

    def get_smoothing_window_size(self) -> int:
        """:return size of the smoothing window to be used"""
        return self.frame_plot_options.get_smoothing_window_size()

    def get_csv_path(self) -> str:
        """:return the path to the selected .csv data file"""
        return self.frame_open_file.get_csv_path()

    def plot_data(self, series, label: str):
        """Plot the data onto the axis"""
        self.frame_figure.plot(series.index.values, series.values, label)

    def set_labels(self):
        """Set x/y-labels and figure title"""
        self.frame_figure.set_x_axis_label(self.frame_plot_options.get_x_axis_label())
        self.frame_figure.set_y_axis_label(self.frame_plot_options.get_y_axis_label())
        self.frame_figure.set_title(self.frame_plot_options.get_figure_title())
        # Save x/y labels to the history
        self.plot_history.set_x_title(self.frame_plot_options.get_x_axis_label())
        self.plot_history.set_y_title(self.frame_plot_options.get_y_axis_label())
        self.plot_history.set_figure_title(self.frame_plot_options.get_figure_title())

    def set_history_paths(self, record: HistoryRecord) -> bool:
        """This function supposes that the .csv data file is located in
        .../experiments/<experiment_name>/data/<platform_name>/<datafile.csv> -> this path is parsed from the
        end to obtain <experiment_name>, <platform_name> and <datafile.csv>, which are set to the record.
        Return True if paths were set."""
        csv_path = self.get_csv_path()
        if os.path.exists(csv_path):
            csv_path_components = os.path.normpath(csv_path).split(os.sep)
            if len(csv_path_components) < 5:
                print("Path {:s} contains less than 5 elements; however, structure like /experiments/<experiment-name>/data/<platform>/<file.csv> was expected.".format(csv_path))
                return False

            file_name = str(csv_path_components[-1])
            platform_name = str(csv_path_components[-2])
            experiment_name = str(csv_path_components[-4])
            if not file_name.endswith(".csv"):
                print("Filename {:s} does not end with '.csv'.".format(csv_path_components[-1]))
                return False
            if csv_path_components[-3] != "data":
                print("Structure like /experiments/<experiment-name>/data/<platform>/<file.csv> was expected, but got {:s} instead.".format(csv_path))
                return False
            if csv_path_components[-5] != "experiments":
                print("Structure like /experiments/<experiment-name>/data/<platform>/<file.csv> was expected, but got {:s} instead.".format(csv_path))
                return False

            record.set_data_paths(experiment_name, platform_name, file_name)
            return True

    def update_figure(self):
        """Update the frame figure"""
        self.frame_figure.update_figure()

    def history_step_back(self):
        """Go back one step in the history"""
        self.plot_history.remove_last()  # Remove the last element
        self.history_plot()  # Plot the history

    def history_plot(self):
        """Redraw the plot according to the current history."""
        self.clear_figure()
        # plot the data stored in the history
        for records in self.plot_history.list_of_records:
            for record in records:
                data_path = record.get_path(self.experiments_root_path)
                if not data_path:
                    print("Data path was not reconstructed, skipping record.")
                    continue
                df = pd.read_csv(data_path, comment="#")

                data_x = df[record.x_col_name] * record.x_scale
                data_y = df[record.y_col_name] * record.y_scale
                lbl = record.y_label

                if record.use_subtract():
                    data_y -= df[record.y_subtract] * record.y_subtract_scale

                if record.use_smoothing():
                    data_y = data_y.rolling(window=record.smoothing_window_size).mean()

                self.plot_data(data_x, data_y, lbl)
        # update the axis
        self.frame_figure.set_x_axis_label(self.plot_history.x_axis_title)
        self.frame_figure.set_y_axis_label(self.plot_history.y_axis_title)
        self.frame_figure.set_title(self.plot_history.figure_title)
        self.update_figure()

    def save_history_to_file(self):
        """Save the plotting history to file."""
        if self.plot_history.history_not_empty():
            file = tkfile.asksaveasfile(mode="w", defaultextension=".txt")
            if file:
                self.plot_history.save_to_file(file)
                file.close()

    def load_history_from_file(self):
        """Load the history from history file"""
        file = tkfile.askopenfile(mode="r", defaultextension=".txt")
        if file:
            self.plot_history.load_from_file(file)

        self.history_plot()

        # set axis labels and figure title
        self.frame_plot_options.set_x_axis_label(self.plot_history.x_axis_title)
        self.frame_plot_options.set_y_axis_label(self.plot_history.y_axis_title)
        self.frame_plot_options.set_figure_title(self.plot_history.figure_title)

        # clear frames
        self.frame_plot_selector.clear_options()
        self.frame_open_file.clear_selection()

    def plot(self):
        """Plot the selected data to the figure."""
        if self.get_clear():
            self.clear()

        df = self.get_df()

        if df is not None:
            lbls = self.get_y_column_labels()
            scales = self.get_y_column_scales()

            records = []

            for col in self.get_y_column_names_selected():
                cur_rec = HistoryRecord()
                lbl = lbls[col]
                scale = scales[col]
                data_y = df[col].dropna() * scale
                subtract_col = self.get_subtract_column()

                if self.get_subtract() and subtract_col:  # subtract the selected column
                    subtract_scale = self.get_subtract_column_scale()
                    y_sub = data_y.interpolate() - df[subtract_col].interpolate() * subtract_scale
                    data_y = y_sub[data_y.dropna().index]  # keep only non-interpolated values

                    cur_rec.set_scale_data(subtract_col, subtract_scale)  # set subtract column in the history

                if self.get_smoothing():  # do the smoothing
                    win_size = self.get_smoothing_window_size()
                    data_y = data_y.rolling(window=win_size).mean()
                    lbl = lbl + "-smooth" + str(win_size)

                    cur_rec.set_smoothing_window_size(win_size)  # set smoothing window size in the history

                if not self.set_history_paths(cur_rec):
                    print("History paths were not set properly.")
                cur_rec.set_x_data(self.get_x_column_name(), 1)  # set history for x-axis
                cur_rec.set_y_data(col, scale, lbl)  # set history for y-axis
                records.append(cur_rec)

                self.plot_data(data_y, lbl)

            self.plot_history.add_records(records)
        else:
            print("No data were loaded.")

        self.set_labels()  # update labels anyway
        self.update_figure()


def parse_commandline():
    import argparse
    p = argparse.ArgumentParser(description='Plot and save graphs from csv.')
    p.add_argument('-p', '--root_path',
                   help='Path to the root of the experiments folder.',
                   dest="root_path",
                   type=str,
                   required=False
                   )
    p.parse_args('-p /root/asd/what'.split())
    return p.parse_args()


# Run the visualizer
if __name__ == "__main__":
    parsed_args = parse_commandline()

    # Set matplotlib fonts
    plt.rc('xtick', labelsize=8)
    plt.rc('ytick', labelsize=8)
    plt.rc('legend', fontsize=8)

    # Create GUI root, and run the application
    root = tk.Tk()
    ThermacVisualizer(root, parsed_args.root_path).pack(side="top", fill="both", expand=True)
    root.mainloop()
