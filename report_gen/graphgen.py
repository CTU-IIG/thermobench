import pandas as pd
from typing import NamedTuple
import argparse
import cufflinks as cf
import plotly
import plotly.graph_objs as go
import scipy
import numpy as np

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

def normalize_common_units(df,c_units):
    for i in range(len(c_units)):

        if c_units[i] == "ms":
            max_time = df.iloc[:,i].max()
            if max_time > 7200000: # more than 2 hours
                c_units[i] = "hours"
                df.iloc[:,i] = df.iloc[:,i]/3600000
            elif max_time > 120000: # more than 2 minutes
                c_units[i] = "minutes"
                df.iloc[:,i] = df.iloc[:,i]/60000
            elif max_time > 2000: # more than 2 seconds
                c_units[i] = "seconds"
                df.iloc[:,i] = df.iloc[:,i]/1000

        if c_units[i] == "mC":
           c_units[i] = "°C"
           df.iloc[:,i] = df.iloc[:,i]/1000

        if c_units[i] == "per-mille":
           c_units[i] = "%"
           df.iloc[:,i] = df.iloc[:,i]/10

        if c_units[i] == "Hz":
           c_units[i] = "MHz"
           df.iloc[:,i] = df.iloc[:,i]/1000000
        if c_units[i] == "kHz":
           c_units[i] = "MHz"
           df.iloc[:,i] = df.iloc[:,i]/1000
        if c_units[i] == "GHz":
           c_units[i] = "MHz"
           df.iloc[:,i] = df.iloc[:,i]*1000

        if c_units[i] == "uV":
           c_units[i] = "mV"
           df.iloc[:,i] = df.iloc[:,i]/1000
        if c_units[i] == "V":
           c_units[i] = "mV"
           df.iloc[:,i] = df.iloc[:,i]*1000

        if c_units[i] == "Instructions executed":
            max_instr = df.iloc[:,i].max()
            pow10 = len(str(int(max_instr)))-1
            c_units[i] = "10^" + str(pow10) + " instructions executed"
            df.iloc[:,i] = df.iloc[:,i]/pow(10,pow10)
    return df,c_units

def read_csv_files(files):
    tables = []
    # Read all csv files
    for (i, file) in enumerate(files):
        print("Reading ",file)
        df = pd.read_csv(file, sep=',')
        df = df.fillna(method='ffill')
        df = df.fillna(method='bfill')
        c_names,c_units = col_names_units(df)
        df.columns = c_names
        df,c_units = normalize_common_units(df,c_units)
        # Benchmark name == name of csv file without extension
        bench_name = file.split('/')[-1][0:-4]
        mt = meas_table(df,c_names,c_units,bench_name)
        tables.append(mt)
    return tables

def plot_df_normal(df,title,x_name,y_name,x_unit,y_unit,c_names):
    x_label = x_name + " (" + x_unit + ")"
    y_label = y_name + " (" + y_unit + ")"

    data = [];
    for col in range(len(df.columns)-1):
        df1 = df.iloc[:,[0,col+1]]
        df1 = df1.dropna()
        data.append({
        'x': df1.iloc[:,0],
        'y': df1.iloc[:,1],
        'name': c_names[col]
        })

    fig = {
        'data' : data,
        'layout': {
            'title': title,
            'xaxis': {'title': x_label},
            'yaxis': {'title': y_label}
        }
    }

    return fig

def exp_trendline_data(data,x,y,a,b,c):
    xtrend = np.linspace(x[0], x[-1], num=500)
    ytrend = a*(1-b*np.exp(-c*xtrend))
    data.append({
        'x': x,
        'y': y,
        'name': c_names[col]
    })
    data.append({
        'x': xtrend,
        'y': ytrend,
        'name': str(c_names[col] + " trendline")
    })
    return data

def plot_df_bar_exp(df,title,x_name,y_name,x_unit,y_unit,c_names):
    x_label = x_name + " (" + x_unit + ")"
    y_label = y_name + " (" + y_unit + ")"

    data = [];
    a = [None] * (len(df.columns)-1)
    b = [None] * (len(df.columns)-1)
    c = [None] * (len(df.columns)-1)
    for col in range(len(df.columns)-1):
        df1 = df.iloc[:,[0,col+1]]
        df1 = df1.dropna()
        ynp = df1.iloc[:,1].to_numpy()
        cf = scipy.optimize.curve_fit(lambda x,a,b,c: a*(1-b*np.exp(-c*x)),  df1.iloc[:,0],  df1.iloc[:,1], p0=(ynp[-1], 2,2))            
        a[col],b[col],c[col] = cf[0]
        #data = exp_trendline_data(data,df1.iloc[:,0].to_numpy(),df1.iloc[:,1].to_numpy(),a[col],b[col],c[col])

    a, ax = zip(*sorted(zip(a, c_names)))
    b, bx = zip(*sorted(zip(a, c_names)))
    c, cx = zip(*sorted(zip(a, c_names)))
    fig = go.Figure(data=[
#        go.Bar(name='max_value', x=cx, y=a),
#        go.Bar(name='B', x=bx, y=b),
        go.Bar(name='exponent_value', x=cx, y=c)
    ],
    layout=go.Layout(title=go.layout.Title(text=title)))
    return fig

def save_plot(file,fig):
    if file.endswith(".html"):
        plotly.offline.plot(fig,auto_open=False,filename=file)
    else:
        plotly.io.write_image(fig, file)

def plot_by_category(mt, cat, y_names, out_dir, out_types, x_axes_str):
    for i in range(len(mt)):
        x_id = header_has_str_idx(mt[i].df,x_axes_str);
        x_str = [mt[i].c_names[j] for j in x_id]
        for x in x_str:
            for c in range(len(cat)):
                # Skip category if it is the same as the x axis
                if cat[c] == x:
                    continue
                c_id = header_has_str_idx(mt[i].df,[x,cat[c]])
                if(len(c_id)<2):
                    print("Skipping category ",cat[c],
                            ", only found one column: ",[mt[i].c_names[c_id[0]]])
                    continue
                
                colnames = [mt[i].c_names[j] for j in c_id[1:]]

                title = y_names[c] + ": " + mt[i].bench_name

                df2 = mt[i].df.iloc[:,c_id]
                
                fig = plot_df_normal(df2,title, mt[i].c_names[c_id[0]], y_names[c],
                        mt[i].c_units[c_id[0]], mt[i].c_units[c_id[1]], colnames)
                
                for ot in out_types:
                    file = (out_dir + "/" + mt[i].bench_name +
                            ":" + x + "-" + y_names[c] + "." + ot)
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


def join_df_by_col(mt, x, colname):

    df = pd.DataFrame()
    xid = header_has_str_idx(mt[0].df,[x])
    xcol = mt[0].df.iloc[:,xid].columns[0]
    
    df.insert(0,xcol,0)

    for i in range(len(mt)):
        xid = header_has_str_idx(mt[i].df,[x])
        if(len(xid)==0): continue
        xid = xid[0]
        cid = mt[i].c_names.index(colname)
        df1 = mt[i].df.iloc[:,[xid,cid]]
        df1.dropna(inplace=True)
        df1.drop_duplicates(subset=[xcol], keep='last', inplace=True)
        df = pd.DataFrame.merge(df,df1, on=xcol, how="outer")
        df.drop_duplicates(subset=[xcol], keep='last',inplace=True)
    df.sort_values(by=df.columns[0],inplace=True)
    return df

def plot_by_column(mt,out_dir,out_types, x_axes_str):

    x_str = []
    x_unit = []
    for i in range(len(mt)):
        x_id = header_has_str_idx(mt[i].df,x_axes_str);
        for j in x_id:
            if mt[i].c_names[j] not in x_str:
                x_str.append(mt[i].c_names[j])
                x_unit.append(mt[i].c_units[j])

    allc = get_all_col_names(mt)

    for k in range(len(x_str)):
        for (j, c) in enumerate(allc):

            if x_str[k] in c or "stdout" in c:
                 continue

            mt_id = col_mt_idx(mt,c)

            colnames = []
            for i in range(len(mt_id)):
                colnames.append(mt[mt_id[i]].bench_name)
           
            mt_in = []
            for i in mt_id:
                if len(header_has_str_idx(mt[i].df,[x_str[k]]))>0: 
                    mt_in.append(mt[i])
            if len(mt_in)==0: continue
            df = join_df_by_col(mt_in, x_str[k], c)
            
            title = c + " - all tests"
            nid = mt[mt_id[0]].c_names.index(c)
            y_unit = mt[mt_id[0]].c_units[nid]

            fig = plot_df_normal(df,title,x_str[k],c,x_unit[k],y_unit,colnames)

            for ot in out_types:
                file =  out_dir + "/all:" + x_str[k] + "-" + c + "." + ot
                print("Saving figure ",file)
                save_plot(file,fig)
    
def main():

    args = parse_commandline()

    mt = read_csv_files(args.csv_file)
    
    # Grouping categories and names of the y axis for categories
    categories,y_names = get_categories_y_names()

    # Html is generated automatically by plotly
    out_types = ["svg","html"]

    # data series containing these strings are plotted as x axes
    x_axes_str = ["time", "work"]

    plot_by_category(mt,categories,y_names,args.out_dir,out_types,x_axes_str)

    plot_by_column(mt,args.out_dir,out_types,x_axes_str)

main()
