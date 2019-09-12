#!/usr/bin/env python 

import os
import argparse

def parse_commandline():
    p = argparse.ArgumentParser(description='Create html report.')
    p.add_argument('-f','--fig_dir', type=str, required=True,
                         help='Path to the figures directory.')
    p.add_argument('-t','--thumbnail_type', type=str, required=True,
                         help='Image type of thumbnails on the main page.')
    p.add_argument('-g','--graph_type', type=str, required=True,
                         help='Image type of graphs.')
    p.add_argument('-o','--out_dir', type=str, required=True,
                         help='Output directory for the report.')
    return p.parse_args()

class Params():
    def __init__(self):
        self.figdir : str = None
        self.outdir : str = None
        self.thumbnail_type : str = None
        self.graph_type : str = None
        self.stylefile : str = None
        self.tests : str = None
        self.categories : str = None
        self.sensors : str = None
        self.all_links : str = None
        self.ind_links : str = None
        self.main_link : str = None

def fill_params(args):
    tests = set()
    categories = set()
    sensors = set()

    for file in os.listdir(args.fig_dir):
        if file.endswith(args.graph_type):
            if file.startswith("all_"):
                sensor = file[4:].split('.')[0]
                sensors.add(sensor)
            else:
                test = '_'.join(file.split('_')[:-1])
                cat = file.split('_')[-1].split('.')[0]
                tests.add(test)
                categories.add(cat)

    params = Params()
    params.figdir = args.fig_dir
    params.outdir = os.path.join(args.out_dir,"htmls")
    if not os.path.exists(params.outdir):
        os.mkdir(params.outdir) 
    params.stylefile = os.path.join(params.outdir,"style.css")
    params.main_link = os.path.join(args.out_dir,"main.html")
    params.graph_type = args.graph_type
    params.thumbnail_type = args.thumbnail_type

    params.sensors = sorted(list(sensors))
    params.tests = sorted(list(tests))
    params.categories = sorted(list(categories))
    params.all_links = all_tests_links(params.sensors,params.outdir)
    params.ind_links = ind_tests_links(params)
    return params    

def header(title,stylefile):
    header="""<html xmlns=\"http://www.w3.org/1999/xhtml\" lang=\"en\" xml:lang=\"en\">
<head>
    <meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\">
    <title>{title}</title>
    <link rel=\"stylesheet\" href=\"{stylefile}\" type=\"text/css\"> 
</head>
<body>
<h1>{title}</h1>
<div class=\"otherview\">
""".format(title=title,stylefile=stylefile) 
    return header

def footer():
    footer="""
</div>
</body>
</html>"""
    return footer

def gen_css(stylefile):
    s = """img { border: 0; }
table { border-collapse: collapse; }
th, td { border: 1px solid lightgray; padding: 4px;}
h4 { margin: 0; }
.otherview { margin: 1ex 0}
.otherview .value { color: black; padding: 0ex 1ex; -moz-border-radius: 1ex; border-radius: 1ex;}
.otherview .value a { color: inherit; text-decoration: none; }
.otherview .other:hover { background: #eee; }
.otherview .missing { color: gray; }
.otherview .current { background: #ccc; }
.na { width: 150px; height: 109px; display:  table-cell; text-align: center; vertical-align: middle; }
left_list {
  float: left;
  width: 15%;
}
right_fig {
  float: right;
  width: 85%;
}
"""
    f = open(stylefile,"w")
    f.write(s)
    f.close()

def sel_list(items,links,header,sel_id):
    sl="<table><tbody><thead><th>" + header + "</th></thead>\n"
    for i in range(len(items)):
        highlight = "value current" if i == sel_id else "value other"
        sl += "<tr><td><span class=\"" + highlight + "\">"
        if links[i] != "" and i != sel_id:
            sl += "<a href=" + links[i] + ">"
        sl += items[i] + "</span>"
        sl += "</td></tr>\n"
    sl+="</tbody></table>\n"
    return sl

def selection_table(p,sel_id,in_main):
    sel_table="""
<table><tbody>
<tr>"""
    l0 = p.all_links[0]
    if not in_main:
        l0 = "../" + l0
    items = ["All tests comparison","Individual tests"]
    links = [l0,str("../" + p.main_link)]
    for i,item in enumerate(items):
        if sel_id == i:
            highlight = "value current"
            link = ""
        else: 
            highlight = "value other"
            link = "<a href=" + links[i] + ">"
        sel_table += "<td><span class=\"" + highlight + "\">"
        sel_table += link + item + "</span></td>\n"
    sel_table+="""
</tr>
</tbody></table>
""".format(st=sel_table)
    return sel_table

def all_tests_body(p,sel_id):
    page = selection_table(p,0,False)
    page += "<left_list>\n"
    links = []
    for l in p.all_links:
        links.append(str("../" + l))
    page += sel_list(p.sensors,links,"sensors",sel_id)
    page += "</left_list>\n\n<right_fig>"
    filename = "../" + p.figdir + "/all_" + p.sensors[sel_id] + "." + p.graph_type
    page += img(filename,"60%","60%")
    page += "</right_fig>"
    return page

def all_tests_links(sensors,basedir):
    links = []
    for sensor in sensors:
        links.append(str(basedir + "/all_" + sensor + ".html"))
    return links

def gen_all_tests_htmls(p):
    for i in range(len(p.all_links)):
        f = open(p.all_links[i],"w")
        page = header("Results",str("../" + p.stylefile))
        page += all_tests_body(p,i)
        page += footer()
        f.write(page)
        f.close()

def test_exists(figdir,test,category,filetype):
    imgsrc = figdir + "/" + test + "_" + category + "." + filetype
    return os.path.isfile(imgsrc)

def ind_tests_links(p):
    links = []
    for t in p.tests:
        l = []
        for c in p.categories:
            tmp = ""
            if test_exists(p.figdir,t,c,p.graph_type):
                tmp = p.outdir + "/" + t + "_" + c + ".html"
            l.append(tmp)
        links.append(l)
    return links  

def ind_tests_main(p):
    page = selection_table(p,1,True)
    page += "<table><thead><tr><td>Category &rarr; <br />Test &darr;</td>"
    for c in p.categories:
        page += "<th>" + c + "</th>"
    page += "</tr></thead>"

    for i,t in enumerate(p.tests):
        page += "<tr><th>" + t + "<br></th>"
        for j,c in enumerate(p.categories):
            if not test_exists(p.figdir,t,c,p.thumbnail_type):
                page += "<td><center>N/A</center></td> "
                continue
            page += "<td><a href=" + p.ind_links[i][j] + ">"
            imgsrc = p.figdir + "/" + t + "_" + c + "." + p.thumbnail_type
            page += img(imgsrc,"100%","auto")
            page += "</a></td>"
        page += "</tr>"
    page += "</table></thead>"
    return page

def img(img,width,height):
    params = " frameborder=\"0\" height=\"" + height + "\" width=\"" + width + "\" "
    if img.endswith(".png") or img.endswith(".svg"):
        s = "<img " + params + "src=" + img + "></img>"
    elif img.endswith(".html") or img.endswith(".pdf"):    
        s = "<script src=\"https://cdn.plot.ly/plotly-latest.min.js\"></script>"
        s += "<iframe " + params + "src=" + img + "></iframe>"
    else:
        print(str("Unknown filetype for " + img + "\nQuitting."))
        exit()
    return s

def ind_test(p,t_id,c_id):
    page = selection_table(p,1,False)
    page += "<table><tr><th>Category</th>"
    for i,c in enumerate(p.categories):
        page += "<td>" 
        highlight = "value current" if i == c_id else "value other"
        page += "<span class=\"" + highlight + "\">"
        if test_exists(p.figdir,p.tests[t_id],c,p.thumbnail_type) and i != c_id:
            page += "<a href=" + "../" + p.ind_links[t_id][i] + ">"
        page += c + "</span>\n</td>"

    page += "</tr></table>"
    tlinks = []
    for i,t in enumerate(p.tests):
        tlinks.append(str("../" + p.ind_links[i][c_id]))
    page += "<left_list>\n"
    page += sel_list(p.tests,tlinks,"Tests",t_id)
    page += "</left_list>\n\n<right_fig>"
    imgsrc = "../" + p.figdir + "/" + p.tests[t_id] + "_" + p.categories[c_id] + "." + p.graph_type
    page += img(imgsrc,"60%","60%")
    page += "</right_fig>"
    page += "<a href=\'../" + p.main_link + "\'>Back to top</a><br/>"
    return page

def gen_ind_tests_htmls(p):

    f = open(p.main_link,"w")
    page = header("Results", p.stylefile)
    page += ind_tests_main(p)
    page += footer()
    f.write(page)
    f.close()

    for i in range(len(p.ind_links)):
        for j in range(len(p.ind_links[i])):
            if p.ind_links[i][j] != "":
                f = open(p.ind_links[i][j],"w")
                page = header("Results",str("../" + p.stylefile))
                page += ind_test(p,i,j)
                page += footer()
                f.write(page)
                f.close()


args = parse_commandline()

params = fill_params(args)

gen_css(params.stylefile)
gen_all_tests_htmls(params)
gen_ind_tests_htmls(params)
