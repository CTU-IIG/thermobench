import pandas as pd
from typing import NamedTuple
import argparse
import cufflinks as cf
import plotly
import plotly.graph_objs as go

smalldata = False

def parse_commandline():
    p = argparse.ArgumentParser(description='Plot and save graphs from csv.')
    p.add_argument('-c','--csv_file',
                         help='Path to the csv file to visualize',
                         type=str,
                         nargs='+',
                         required=True
                         )
    p.add_argument('-o','--out_dir',
                         help='Path to the output directory.',
                         type=str,
                         default='./'
                         )
    return p.parse_args()

def get_categories_y_names():
    if smalldata:
        categories = ["L","gpu_work","FALALA"]
        y_names = ["Temperature","GPU work done","fala"]
    else:
        categories = ["temp","load","freq","current","voltage","power","cpu_work_done"]
        y_names = ["Temperature","Load","Frequency","Current","Voltage","Power","CPU work done"]
    return categories,y_names


class meas_table(NamedTuple):
    df: pd.DataFrame
    c_names: list
    c_units: list
    bench_name: str

def col_names_units(df):
    names = []
    units = []
    for c in df.columns:
        s = c.strip().split('/')
        names.append(s[0])
        units.append(s[1] if len(s) > 1 else "no unit")
    return names,units

# Returns indices of columns whose header contains
# at least 1 of the strings
# There should be a better, regex way to do this, but I couldn't find it
# TODO: Needs a better name
def header_has_str_idx(df, strings):
    indices = []
    for (i,s) in enumerate(strings):
        cols = [col for col in df.columns if s in col]
        for c in cols:
            indices.append(df.columns.get_loc(c))
    return indices

def read_csv_files(files):
    tables = []
    # Read all csv files
    for (i, file) in enumerate(files):
        print("Reading ",file)
        df = pd.read_csv(file, sep=',')
        with pd.option_context('display.width',None,'display.max_rows', None, 'display.max_columns', None):
            print(df)
        df = df.fillna(method='ffill')
        with pd.option_context('display.width',None,'display.max_rows', None, 'display.max_columns', None):
            print(df)
        print(df.dtypes)
        if len(header_has_str_idx(df, ["time"])) == 0:
            print(file," has no time column. Skipping file.")
            continue
        c_names,c_units = col_names_units(df)
        # Benchmark name == name of csv file without extension
        bench_name = file.split('/')[-1][0:-4]
        mt = meas_table(df,c_names,c_units,bench_name)
        tables.append(mt)
    return tables

def plot_df(df,title,x_name,y_name,x_unit,y_unit,c_names):
    x_label = x_name + " (" + x_unit + ")"
    y_label = y_name + " (" + y_unit + ")"

    fig = {
        'data': [{
        'x': df.iloc[:,0],
        'y': df.iloc[:,col+1],
        'name': c_names[col]
        }  for col in range(len(df.columns)-1)],

        'layout': {
            'title': title,
            'xaxis': {'title': x_label},
            'yaxis': {'title': y_label}
        }
    }
    return fig

def save_plot(file,fig):
    if file.endswith(".html"):
        plotly.offline.plot(fig,auto_open=False,
                            filename=file)
    else:
        plotly.io.write_image(fig, file)

def plot_by_category(mt, cat, y_names, out_dir, out_types):
    for i in range(len(mt)):
        for c in range(len(cat)):

            c_id = header_has_str_idx(mt[i].df,["time",cat[c]])
            if(len(c_id)<2):
                print("Skipping category ",cat[c],
                        ", only found one column: ",[mt[i].c_names[j] for j in c_id])
                continue
            
            colnames = [mt[i].c_names[j] for j in c_id[1:]]

            title = y_names[c] + ": " + mt[i].bench_name

            df2 = mt[i].df.iloc[:,c_id]
            
            fig = plot_df(df2,title, mt[i].c_names[c_id[0]], y_names[c],
                    mt[i].c_units[c_id[0]], mt[i].c_units[c_id[1]], colnames)
            
            for ot in out_types:
                file = (out_dir + "/" + mt[i].bench_name +
                        "_" + y_names[c] + "." + ot)
                print("Saving figure ",file)
                save_plot(file,fig)

def get_all_col_names(mt):
    c = set()
    for i in range(len(mt)):
        c |= set(mt[i].c_names)
    return sorted(list(c))

def col_mt_idx(mt,colname):
    idx = []
    for i in range(len(mt)):
        if colname in mt[i].c_names:
            idx.append(i)
    return idx


def join_df_by_col(mt, colname):

        df = pd.DataFrame()
        tid = header_has_str_idx(mt[0].df,["time"])
        timecol = mt[0].df.iloc[:,tid].columns[0]
        
        df.insert(0,timecol,0)
        
        for i in range(len(mt)):
            tid = header_has_str_idx(mt[i].df,["time"])[0]
            cid = mt[i].c_names.index(colname)
            df1 = mt[i].df.iloc[:,[tid,cid]]
            df1 = df1.dropna()
            df = pd.DataFrame.merge(df,df1, on=timecol, how="outer")
        return df

def plot_by_column(mt,out_dir,out_types):

    t_idx = header_has_str_idx(mt[0].df,["time"])[0]
    t_name = mt[0].c_names[t_idx]
    t_unit = mt[0].c_units[t_idx]

    allc = get_all_col_names(mt)

    for (j, c) in enumerate(allc):

        if "time" in c or "stdout" in c:
             continue

        mt_id = col_mt_idx(mt,c)

        colnames = []
        for i in range(len(mt_id)):
            colnames.append(mt[mt_id[i]].bench_name)
        
        df = join_df_by_col([mt[i] for i in mt_id], c)
        
        title = c + " - all tests"
        nid = mt[mt_id[0]].c_names.index(c)
        y_unit = mt[mt_id[0]].c_units[nid]

        fig = plot_df(df,title,t_name,c,t_unit,y_unit,colnames)

        for ot in out_types:
            file =  out_dir + "/all_" + c + "." + ot
            print("Saving figure ",file)
            save_plot(file,fig)
    
def main():

    args = parse_commandline()

    mt = read_csv_files(args.csv_file)
    
    # Grouping categories and names of the y axis for categories
    categories,y_names = get_categories_y_names()

    # Html is generated automatically by plotly
    out_types = ["svg","html"]

    plot_by_category(mt,categories,y_names,args.out_dir,out_types)

    plot_by_column(mt,args.out_dir,out_types)

main()
