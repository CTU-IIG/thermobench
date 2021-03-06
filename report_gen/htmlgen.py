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
        self.x_names : str = None
        self.all_links : str = None
        self.ind_links : str = None
        self.ind_main_links : str = None
        self.main_link : str = None

def fill_params(args):
    tests = set()
    categories = set()
    sensors = set()
    x_names = set()

    for file in os.listdir(args.fig_dir):
        if file.endswith(args.graph_type):
            file = file.replace(" ", "%20")
            if file.startswith("all:"):
                fn = file[4:].split('.')[0].split('-')
                x_name = fn[0]
                sensor = fn[1]
                sensors.add(sensor)
                x_names.add(x_name)
            else:
                fn = file.split(':')
                test = fn[0]
                x_name = fn[1].split('-')[0] 
                cat = fn[1].split('-')[1] 
                cat = cat.split('.')[0]
                tests.add(test)
                categories.add(cat)
                x_names.add(x_name)

    params = Params()
    params.figdir = args.fig_dir
    params.outdir = os.path.join(args.out_dir,"htmls")
    if not os.path.exists(params.outdir):
        os.mkdir(params.outdir) 
    params.stylefile = "style.css"
    params.main_link = os.path.join(args.out_dir,"main.html")
    params.graph_type = args.graph_type
    params.thumbnail_type = args.thumbnail_type

    params.sensors = sorted(list(sensors))
    params.tests = sorted(list(tests))
    params.categories = sorted(list(categories))
    params.x_names = sorted(list(x_names))
    params.all_links = all_tests_links(params)
    params.ind_links = ind_tests_links(params)
    params.ind_main_links = ind_main_links(params)
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
    return "</div></body></html>"

def gen_css(stylefile):
    s = """    img { border: 0; }
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
    }"""
    f = open(stylefile,"w")
    f.write(s)
    f.close()

def sel_list(items,links,header,sel_id):
    sl="<table><tbody><thead><th>" + header + "</th></thead>\n"
    for i in range(len(items)):
        if not links[i]:
            continue
        highlight = "value current" if i == sel_id else "value other"
        sl += "<tr><td><span class=\"" + highlight + "\">"
        if i != sel_id:
            sl += "<a href=" + links[i] + ">"
        sl += items[i].replace("%20"," ") + "</span>"
        sl += "</td></tr>\n"
    sl+="</tbody></table>\n"
    return sl

def sel_row(title,items,links,sel_id):
    sr = "<tr>"
    if title:
        sr +="<th>" + title + "</th>"
    for i,item in enumerate(items):
        if not links[i]:
            continue
        if sel_id == i:
            highlight = "value current"
            link = ""
        else: 
            highlight = "value other"
            link = "<a href=" + links[i] + ">"
        sr += "<td><span class=\"" + highlight + "\">"
        sr += link + item.replace("%20"," ") + "</span></td>\n"
    sr += "</tr>"
    return sr

# sel hold the ids of the selected options in the selection table
# 
# helper_sel holds ids of other selected options not in the
# selection table, but needed to get the correct links
# (e.g. selected test id in the list at the all tests comparison page)
def selection_table(p,sel,helper_sel):

    sel_table="<table><tbody>"
    items = ["All tests comparison","Individual tests"]

    l0 = next((x for i,x in enumerate(p.all_links[sel[1]]) if x), None)
    links = [l0,p.ind_main_links[sel[1]]]
    sel_table += sel_row("",items,links,sel[0])

    if sel[0] == 0:
        links = [p.all_links[i][helper_sel[0]]  
                 for i in range(len(p.all_links))]
    elif len(sel)==2:
        links = p.ind_main_links
    else:
        links = [p.ind_links[i][helper_sel[0]][helper_sel[1]] 
                 for i in range(len(p.x_names))]

    sel_table += sel_row("x axis",p.x_names,links,sel[1])
    sel_table += "</tbody></table>".format(st=sel_table)

    return sel_table

def all_tests_body(p,s_id,x_id):
    page = selection_table(p,[0,x_id],[s_id])
    page += "<left_list>\n"
    page += sel_list(p.sensors,p.all_links[x_id],"sensors",s_id)
    page += "</left_list>\n\n<right_fig>"
    filename = "../" + p.figdir + "/all:" + p.x_names[x_id] + "-" + p.sensors[s_id] + "." + p.graph_type
    page += img(filename,"60%","60%")
    page += "</right_fig>"
    return page

def all_tests_links(p):
    links = []
    for x in p.x_names:
        l = []
        for s in p.sensors:
            if test_exists(p.figdir,"all",x,s,p.graph_type):
                l.append(str("./all:" + x + "-" + s + ".html"))
            else:
                l.append("")
        links.append(l)
    return links

def gen_all_tests_htmls(p):
    for x_id in range(len(p.x_names)):
        for i in range(len(p.all_links[x_id])):
            link = p.all_links[x_id][i].replace("%20"," ")
            if link:
                f = open(os.path.join(p.outdir,link),"w")
                page = header("Results",str(p.stylefile))
                page += all_tests_body(p,i,x_id)
                page += footer()
                f.write(page)
                f.close()

def test_exists(figdir,test,x_name,category,filetype):
    imgsrc = figdir + "/" + test + ":" + x_name + "-" + category + "." + filetype
    imgsrc = imgsrc.replace("%20"," ")
    return os.path.isfile(imgsrc)

def ind_main_links(p):
    links = []
    for x in p.x_names:
        links.append(str("./main_" + x + ".html"))
    return links

def ind_tests_links(p):
    links = []
    for x in p.x_names:
        ll = []
        for t in p.tests:
            l = []
            for c in p.categories:
                tmp = ""
                if test_exists(p.figdir,t,x,c,p.graph_type):
                    tmp = "./" + t + ":" + x + "-" + c + ".html"
                l.append(tmp)
            ll.append(l)    
        links.append(ll)
    return links  

def ind_tests_main(p,x_id):
    page = selection_table(p,[1,x_id],[])
    page += "<table><thead>"

    page += "<tr><td>Category &rarr; <br />Test &darr;</td>"
    for c in p.categories:
        page += "<th>" + c.replace("%20"," ") + "</th>"
    page += "</tr></thead>"

    x = p.x_names[x_id]
    for i,t in enumerate(p.tests):
        page += "<tr><th>" + t + "<br></th>"
        for j,c in enumerate(p.categories):

            if not test_exists(p.figdir,t,x,c,p.thumbnail_type):
                page += "<td><center>N/A</center></td>"
                continue

            page += "<td><a href=" + p.ind_links[x_id][i][j] + ">"
            imgsrc = ("../" + p.figdir + "/" + t + ":" 
                      + x + "-" + c + "." + p.thumbnail_type)
            page += img(imgsrc,"100%","auto")
            page += "</a></td>"

        page += "</tr>"
    page += "</table>"
    return page

def img(img,width,height):
    params = " frameborder=\"0\" height=\"" + height 
    params += "\" width=\"" + width + "\" "

    if img.endswith(".png") or img.endswith(".svg"):
        s = "<img " + params + "src=" + img + "></img>"
    elif img.endswith(".html") or img.endswith(".pdf"):    
        s = "<script src=\"https://cdn.plot.ly/plotly-latest.min.js\"></script>"
        s += "<iframe " + params + "src=" + img + "></iframe>"
    else:
        print(str("Unknown filetype for " + img + "\nQuitting."))
        exit()
    return s

def ind_test(p,x_id,t_id,c_id):
    page = selection_table(p,[1,x_id],[t_id,c_id])

    page += "<table><tr><th>Category</th>"
    for i,c in enumerate(p.categories):

        if not p.ind_links[x_id][t_id][i]:
            continue

        page += "<td>" 
        highlight = "value current" if i == c_id else "value other"
        page += "<span class=\"" + highlight + "\">"

        if test_exists(p.figdir,p.tests[t_id],p.x_names[x_id],
                       c,p.thumbnail_type) and i != c_id:
            page += "<a href=" + p.ind_links[x_id][t_id][i] + ">"
        page += c.replace("%20"," ") + "</span>\n</td>"

    page += "</tr></table>"

    tlinks = []
    for i,t in enumerate(p.tests):
        tlinks.append(p.ind_links[x_id][i][c_id])
    page += "<left_list>\n"
    page += sel_list(p.tests,tlinks,"Tests",t_id)
    page += "</left_list>\n\n"

    page += "<right_fig>"
    imgsrc = ("../" + p.figdir + "/" + p.tests[t_id] + ":" 
             + p.x_names[x_id] + "-" + p.categories[c_id] 
             + "." + p.graph_type)
    page += img(imgsrc,"60%","60%")

    page += "</right_fig><a href=\'" + p.ind_main_links[x_id]
    page += "\'>Back to top</a><br/>"
    return page

def gen_ind_tests_htmls(p):

    # Main link just a redirect
    f = open(p.main_link,"w")
    page = "<html><head><meta http-equiv=\"Refresh\" content=\"1; url="
    page += os.path.join(p.outdir,p.ind_main_links[0]) 
    page += "\"></head><body></body></html>"
    f.write(page)
    f.close()

    for x_id in range(len(p.x_names)):
        link = p.ind_main_links[x_id].replace("%20"," ")
        f = open(os.path.join(p.outdir,link),"w")
        page = header("Results", p.stylefile)
        page += ind_tests_main(p,x_id)
        page += footer()
        f.write(page)
        f.close()


        for k in range(len(p.ind_links)):
            for i in range(len(p.ind_links[k])):
                for j in range(len(p.ind_links[k][i])):
                    if p.ind_links[k][i][j] != "":
                        link = p.ind_links[k][i][j].replace("%20"," ")
                        f = open(os.path.join(p.outdir,link),"w")
                        page = header("Results",p.stylefile)
                        page += ind_test(p,k,i,j)
                        page += footer()
                        f.write(page)
                        f.close()


args = parse_commandline()

params = fill_params(args)

gen_css(os.path.join(params.outdir,params.stylefile))
gen_all_tests_htmls(params)
gen_ind_tests_htmls(params)
