import pandas as pd
from recordclass import recordclass, RecordClass
import argparse
import cufflinks as cf
import plotly
import plotly.graph_objs as go
import scipy
import numpy as np
import json
import os

def parse_commandline():
    p = argparse.ArgumentParser(description='Plot and save graphs from csv.')
    p.add_argument('-f','--csv_file',
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

    p.add_argument('--plot_col',
                         help='Only plot the supplied columns with plot_by_column.',
                         type=str,
                         nargs='+',
                         default='all'
                         )

    p.add_argument('-x','--x_axes_str',
                         help='X axes will be all columns containing these strings.',
                         type=str,
                         nargs='+',
                         default=None
                         )

    p.add_argument('-e','--extension',
                         help='Extension of the exported graphs.',
                         type=str,
                         nargs='+',
                         default=['svg']
                         )

    p.add_argument('-c','--categories',
                         help='Category search string and category name pairs for grouping graphs. E.g. \'{\"temp\":\"Temperature\",\"freq\":\"Frequency\"}\'',
                         type=json.loads,
                         default={}
                         )

    return p.parse_args()

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

def col_names_units(df):
    names = []
    units = []
    for c in df.columns:
        s = c.strip().split('/')
        names.append(s[0])
        units.append(s[1] if len(s) > 1 else "")
    return names,units

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
            c_units[i] = "10^9 instructions executed"
            df.iloc[:,i] = df.iloc[:,i]/pow(10,9)
    return df,c_units


def read_csv_files(files):
    dfs = []
    for (i, file) in enumerate(files):
        print("Reading ",file)
        df = pd.read_csv(file, sep=',',comment='#')
        df = df.fillna(method='ffill')
        df = df.fillna(method='bfill')
        c_names,c_units = col_names_units(df)
        df,c_units = normalize_common_units(df,c_units)
        for i in range(len(df.columns)):
            s = c_names[i]
            if c_units[i]: 
                s += " (" + c_units[i] + ")"
            df.rename(columns={df.columns[i]: s}, inplace = True)
        df.name = file.split('/')[-1][0:-4] # basename of csv file without extension
        dfs.append(df)
    return dfs

def plot_df_normal(df,title,x_label,y_label,c_names):

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
    fig = go.Figure(data=[go.Bar(name='exponent_value', x=cx, y=c)],
                    layout=go.Layout(title=go.layout.Title(text=title)))
    return fig

def save_plot(file,fig):
    if file.endswith(".html"):
        plotly.offline.plot(fig,auto_open=False,filename=file)
    else:
        plotly.io.write_image(fig, file)

def plot_by_category(dfs, cat, out_dir, out_types, x_axes_str):
    for i in range(len(dfs)):
        x_id = header_has_str_idx(dfs[i],x_axes_str);
        x_str = [dfs[i].columns[j] for j in x_id]
        for x in x_str:
            for c in cat:
                # Skip category if it is the same as the x axis
                if c == x:
                    continue
                c_id = header_has_str_idx(dfs[i],[x,c])
                if(len(c_id)<2):
                    print("Skipping category ",c,
                            ", only found one column: ",[dfs[i].columns[c_id[0]]])
                    continue
                
                colnames = [dfs[i].columns[j] for j in c_id[1:]]

                title = cat[c] + ": " + dfs[i].name

                df2 = dfs[i].iloc[:,c_id]
                
                fig = plot_df_normal(df2,title, dfs[i].columns[c_id[0]], cat[c], colnames)
                
                for ot in out_types:
                    xy = x.split(" ")[0] + "-" + cat[c].split(" ")[0]
                    file = (out_dir + "/" + dfs[i].name +
                            ":" + xy + "." + ot)
                    print("Saving figure ",file)
                    save_plot(file,fig)

def get_all_col_names(dfs):
    c = set()
    for i in range(len(dfs)):
        c |= set(dfs[i].columns)
    return sorted(list(c))

def col_dfs_idx(dfs,colname):
    idx = []
    for i in range(len(dfs)):
        if colname in dfs[i].columns:
            idx.append(i)
    return idx


def join_df_by_col(dfs, x, colname):

    df = pd.DataFrame()
    xid = header_has_str_idx(dfs[0],[x])
    xcol = dfs[0].iloc[:,xid].columns[0]
    
    df.insert(0,xcol,0)

    for i in range(len(dfs)):
        xid = header_has_str_idx(dfs[i],[x])
        if(len(xid)==0): continue
        xid = xid[0]
        cid = list(dfs[i].columns).index(colname)
        df1 = dfs[i].iloc[:,[xid,cid]]
        df1.dropna(inplace=True)
        df1.drop_duplicates(subset=[xcol], keep='last', inplace=True)
        df = pd.DataFrame.merge(df,df1, on=xcol, how="outer")
        df.drop_duplicates(subset=[xcol], keep='last',inplace=True)
    df.sort_values(by=df.columns[0],inplace=True)
    return df

def plot_by_column(dfs,out_dir,out_types, x_axes_str):

    x_str = []
    for i in range(len(dfs)):
        x_id = header_has_str_idx(dfs[i],x_axes_str);
        for j in x_id:
            if dfs[i].columns[j] not in x_str:
                x_str.append(dfs[i].columns[j])

    allc = get_all_col_names(dfs)

    for k in range(len(x_str)):
        for (j, c) in enumerate(allc):

            if x_str[k] in c or "stdout" in c:
                 continue

            dfs_id = col_dfs_idx(dfs,c)

            colnames = []
            for i in range(len(dfs_id)):
                colnames.append(dfs[dfs_id[i]].name)
           
            dfs_in = []
            for i in dfs_id:
                if len(header_has_str_idx(dfs[i],[x_str[k]]))>0:
                    dfs_in.append(dfs[i])
            if len(dfs_in)==0: continue

            df = join_df_by_col(dfs_in, x_str[k], c)
            title = c + " - all tests"

            fig = plot_df_normal(df,title,x_str[k],c,colnames)

            for ot in out_types:
                xy = x_str[k].split(" ")[0] + "-" + c.split(" ")[0]
                file =  out_dir + "/all:" + xy + "." + ot
                print("Saving figure ",file)
                save_plot(file,fig)

def prune_cols(dfs,cols,x_axes_str):
    for i in range(len(dfs)):
        cols.extend(x_axes_str)
        ind = header_has_str_idx(dfs[i],cols)
        ind = list(set(ind)) # selected column may be also x axis
        name = dfs[i].name
        dfs[i] = dfs[i].iloc[:,ind]
        dfs[i].name = name
    return dfs

def main():
    args = parse_commandline()

    if not os.path.exists(args.out_dir):
        os.makedirs(args.out_dir)

    dfs = read_csv_files(args.csv_file)

    if args.x_axes_str == None:
        args.x_axes_str = [dfs[0].columns[0]]

    if(args.plot_col != "all"):
        dfs = prune_cols(dfs,args.plot_col,args.x_axes_str)

    plot_by_column(dfs,args.out_dir,args.extension,args.x_axes_str)

    plot_by_category(dfs,args.categories,args.out_dir,args.extension,args.x_axes_str)

main()
