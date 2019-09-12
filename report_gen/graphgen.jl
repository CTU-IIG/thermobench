using ArgParse
using CSV
using Plots
using StatsPlots
using Query
using DataFrames
plotlyjs()

smalldata = false

function parse_commandline(ARGS)

    s = ArgParseSettings()

    @add_arg_table s begin
        "--csv_file", "-c"
            help = "Path to the csv file to visualize"
            arg_type = String
            nargs = '+'
            required = true
        "--out_dir", "-o"
            help = "Path to the output directory."
            arg_type = String
            default = "./"
    end

    return parse_args(ARGS,s)
end

function get_categories_y_names()

    if(smalldata)
        categories = ["L","gpu_work","FALALA"]
        y_names = ["Temperature","GPU work done","fala"]
    else
        categories = ["temp","load","freq","current","voltage","power","cpu_work_done"]
        y_names = ["Temperature","Load","Frequency","Current","Voltage","Power","CPU work done"]
    end
    
    return categories,y_names
end

function col_names_units(file)
    df = CSV.read(file; delim = ',', header = false, limit = 1)
    names = String[]
    units = String[]
    for i = 1:size(df,2)
        s = split(strip(string(df[1,i])),'/')
        push!(names,s[1])
        push!(units,(size(s,1) > 1 ? s[2] : "no unit"))
    end
    return names,units
end

struct meas_table
    df::DataFrame
    c_names::Array{String}
    c_units::Array{String}
    bench_name::String
end

# Input: Array of file paths
# Output: Array of measurement tables
function read_csv_files(files)

    tables = meas_table[]

    # Read all csv files
    for (i, file) in enumerate(files)
        println("Reading $file")
        df=CSV.read(file; delim = ',', normalizenames=true)

        if (size(header_has_str_idx(df, ["time"]),1) == 0)
            println("$file has no time column. Skipping file.")
            continue
        end

        c_names,c_units = col_names_units(file)
        # Benchmark name == name of csv file without extension
        bench_name = split(file,'/')[end][1:end-4]

        mt = meas_table(df,c_names,c_units,bench_name)
        push!(tables,mt)
    end

    return tables
end

# Returns indices of columns whose header contains
# at least 1 of the strings
# There should be a better, regex way to do this, but I couldn't find it
# TODO: Needs a better name
function header_has_str_idx(df, strings)
    indices = []
    for (i,s) in enumerate(strings)
        for (j,c) in enumerate(names(df))
            if (occursin(s, string(c)) && !(j in indices))
                push!(indices, j)
            end
        end
    end
    return indices
end

function plot_df(df,title,x_name,y_name,x_unit,y_unit,c_names)
    x_label = x_name * " (" * x_unit * ")"
    y_label = y_name * " (" * y_unit * ")"

    c_names = reshape(c_names, 1, :)

    @df df plot(cols(1),cols(2:size(df,2)),
                    title = title,
                    xaxis = x_label,
                    yaxis = y_label,
                    label = c_names,
                    hover = cols(2:size(df,2)), 
                    size = (800,500),
                    grid = "on")
end

function plot_by_category(mt, cat, y_names, out_dir, out_types)
    for i = 1:length(mt)
        for c = 1:length(cat)

            c_id = header_has_str_idx(mt[i].df,["time",cat[c]])
            if(size(c_id,1)<2)
                println("Skipping category ",cat[c],
                        ", only found one column: ",mt[i].c_names[c_id])
                continue
            end

            colnames = mt[i].c_names[c_id[2:end]]
            title = y_names[c] * ": " * mt[i].bench_name

            df2 = mt[i].df[:,c_id]
            plot_df(df2, title, mt[i].c_names[1], y_names[c],
                    mt[i].c_units[1], mt[i].c_units[c_id[2]], colnames) 

            for ot in out_types
                file = out_dir * "/" * mt[i].bench_name *
                        "_" * y_names[c] * "." * ot
                println("Saving figure ",file)
                savefig(file)
            end

        end
    end
end

function get_all_col_names(mt)
    c = String[]
    for i=1:size(mt)[1]
        c = union(c,mt[i].c_names)
    end
    return c
end

function col_mt_idx(mt,colname)
    idx = []
    for i = 1:length(mt)
        if(colname in mt[i].c_names)
            push!(idx,i)
        end
    end
    return idx
end

function join_df_by_col(mt, colname)

        df = DataFrame()
        timesymbol = names(mt[1].df[:,header_has_str_idx(mt[1].df,["time"])])[1]
        insertcols!(df, 1, timesymbol => [mt[1].df[1,timesymbol]] )

        for i = 1:length(mt)
            tid = header_has_str_idx(mt[i].df,["time"])[1]
            cid = findfirst(isequal(colname),mt[i].c_names)
            df1 = mt[i].df[:,[tid,cid]]
            df1 = df1[completecases(df1), :]
            df = join(df,df1, kind = :outer, on = timesymbol, makeunique=true)
        end

        return df

end

function plot_by_column(mt,out_dir,out_types)

    t_idx = header_has_str_idx(mt[1].df,["time"])[1]
    t_name = mt[1].c_names[t_idx]
    t_unit = mt[1].c_units[t_idx]

    allc = get_all_col_names(mt)
   
    for (j, c) in enumerate(allc)
    
        if (occursin("time",c) || occursin("stdout",c))
             continue
        end

        mt_id = col_mt_idx(mt,c)

        colnames = []
        for i = 1:length(mt_id)
            push!(colnames,mt[mt_id[i]].bench_name)
        end

        df = join_df_by_col(mt[mt_id], c)

        title = c * " - all tests"
        nid = findfirst(isequal(c),mt[mt_id[1]].c_names)
        y_unit = mt[mt_id[1]].c_units[nid]

        plot_df(df,title,t_name,c,t_unit,y_unit,colnames)

        for ot in out_types
            file =  out_dir * "/all_" * c * "." * ot
            println("Saving figure ",file)
            savefig(file)
        end

    end

end

function main(ARGS)
    println("RUNNING MY MAIN")
    args = parse_commandline(ARGS)

    meas_tables = read_csv_files(args["csv_file"])
    if (size(meas_tables,1) == 0)
        println("Nothing to plot. Quitting.")
        return
    end
     
    # Grouping categories and names of the y axis for categories
    categories,y_names = get_categories_y_names()

    out_types = ["svg","html"]

    # Plots for each test by category
    plot_by_category(meas_tables,categories,y_names,args["out_dir"],out_types)
    
    # Plots for each column by test 
    plot_by_column(meas_tables,args["out_dir"],out_types)

    # TODO: implement plotting of graphs with 2 y axes, to compare
    #       temperature data with other(load, freq, etc.) data
end

Base.@ccallable function julia_main(ARGS::Vector{String})::Cint
  println("RUNNING JULIAMAIN")
  main(ARGS)
  return 0
end

main(ARGS)
