#!/usr/bin/env python
# coding: utf-8

# In[41]:


import pandas as pd
import re


# In[3]:


with open('TTesterLCD32.c', 'r') as f:
    source = f.readlines()

# find the tube_list
prev_line = None
catalog_found = False
curr_line = 0

tube_list = ''

for i, l in enumerate(source):
    # set catalog found but don't include the declaration
    if 'lamprom[FLAMP]' in l:
        catalog_found = True
        continue

    # if empty line or just opening brace - skip
    if '{\n' in l or l == '\n':
        continue

    # have we reached the end of the catalog declaration?
    if "};" in l:
        catalog_found = False

    # just append it already, Tom!
    if catalog_found:
        tube_list += l


# In[55]:


# Russian-style receiver tubes: digit, letters, digit(s), letter
RUSSIAN_P = re.compile(r'^\d+[A-Z]+\d+[PS]$')

def normalize_tube(raw: str) -> str:
    # strip quotes and trailing underscores
    core = raw.strip("'").rstrip('_')

    # keep Russian-style P or S suffix
    if RUSSIAN_P.match(core):
        return core

    # otherwise, if it ends with section marker P/T, drop it e.g. ECL86P/T
    if core.endswith(('P', 'T')):
        return core[:-1]

    # if no match above -- return core
    return core


# In[56]:


def flag_empty_slots(row: list) -> int:
    # flag 
    # -1: power supply and remote on top
    #  0: legit tube
    #  1: placeholder for future additions

    # legit tubes have values for Ua and other params
    # while the placeholders have 0s 
    s = sum([int(e) for e in row[:-2]])

    if row[-1] in ['PWRSU','REMOTE']: # PWRSU cause we trim Ps in normalize_tube!
        return -1
    elif s > 0:
        return 0
    else: return 1


# In[ ]:


def format_fixed_width_with_commas(df, max_width=20):
    # calc width per column
    col_widths = {}
    for col in df.columns:
        data_w = df[col].astype(str).map(len).max()
        col_widths[col] = min(data_w, max_width)

    # formatter
    def fmt_row(row_like):
        parts = []
        for col in df.columns:
            val = str(row_like[col])
            w = col_widths[col]

            # truncate and left‑align
            parts.append(f"{val[:w]:<{w}}")
        return f'{{{",".join(parts)}}},'

    # final output
    lines = []
    header = {col: col for col in df.columns}
    for _, row in df.iterrows():
        lines.append(fmt_row(row))

    return "\n".join(lines)


# In[ ]:


records = [record[1:-2].split(',') for record in tube_list.split('\n')[0:-1]]
records = [r + [''.join([re.strip("'") for re in r[0:6]])] for r in records]
records = [r + [normalize_tube(r[-1])] for r in records]
records = [r + [flag_empty_slots(r[-10:])] for r in records]
df = pd.DataFrame(records).sort_values(by=[21, 20, 7])

tubes = format_fixed_width_with_commas(df.iloc[:, 0:19])
print(text)


# In[5]:


get_ipython().system('jupyter nbconvert --to script tube_list_sort.ipynb')

