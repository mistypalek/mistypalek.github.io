"""
Created on Sun April 9 2023

@author: mistypalek_snhu
"""

import dash
import dash_leaflet as dl
import dash_core_components as dcc
import dash_html_components as html
import dash_table
from dash.dependencies import Input, Output, State
import pandas as pd
import plotly.express as px
import base64
from CRUDHandler import CRUDHandler

###########################
# Data Manipulation / Model
###########################

username = ""
password = ""
shelter = CRUDHandler(username, password)

df = pd.DataFrame.from_records(shelter.findDocs({})).drop(('_id'), axis=1)
app = dash.Dash(__name__)

# Grazioso Salvare’s logo
image_filename = 'GS_logo.png'
encoded_image = base64.b64encode(open(image_filename, 'rb').read())

app.layout = html.Div([
    # header
    html.Center(html.B(html.H1('Misty Palek', style={'color': 'green'}))),
    html.Center(html.B(html.H1('SNHU CS-340 Dashboard', style={'color': 'blue'}))),
    # logo
    html.Center(html.Img(src="data:image/png;base64,{}".format(encoded_image.decode()))),
    html.Hr(),

    # div to hold radio items for filtering by resuce type
    html.Div([
        html.Center(html.Label(['Select an option to filter the table by rescue type:'], style={'font-weight': 'bold'})),
        html.Center(dcc.RadioItems(
            id='radio-items',
            options=[
                {'label': 'Water Rescue', 'value': 'Filter by Water Rescue'},
                {'label': 'Mountain Rescue', 'value': 'Filter by Mountain Rescue'},
                {'label': 'Disaster Rescue', 'value': 'Filter by Disaster Rescue'},
                {'label': 'Reset', 'value': 'Remove all filters'}
            ],
            value='Filter table by rescue dog types',
            inputStyle={"margin-left": "20px"},
            labelStyle={'display': 'inline-block'}
        ))
    ]),
    html.Br(),

    # create the data table
    dash_table.DataTable(
        id='datatable-interactivity',

        # for every column, set the name and id. also set column options
        columns=[
            {"name": i, "id": i, "deletable": False, "selectable": True} for i in df.columns
        ],

        # populate table with records from DataFrame
        data=df.to_dict('records'),

        # set data table interactivity
        editable=False,
        filter_action="native",
        sort_action='custom',
        sort_mode="multi",
        sort_by=[],
        row_selectable="single",
        row_deletable=True,
        selected_columns=[],
        selected_rows=[],
        page_action="native",
        page_current=0,
        page_size=10
    ),

    html.Br(),
    html.Hr(),
    # This div sets up the dashboard so that your chart and your geolocation chart are side-by-side
    html.Div(className='row',
            style={'display': 'flex', "width": "500"},
            children=[
                html.Div(
                    className='row',
                    style={'display': 'flex'},
                    children=[
                        # where the pie chart will go
                        dcc.Graph(
                            id='graph-id',
                        ),
                        # where the map will go
                        html.Div(
                            id='map-id',
                            className='col s12 m6',
                        ),
                    ]
                ),
            ])
])



@app.callback(
    Output('datatable-interactivity', 'data'),
    [Input(component_id='radio-items', component_property='value')
    ])
def radioFilter(radio_options):
    """ functionality for radio items. update data table with rescue type filters """
    if radio_options == 'Filter by Water Rescue':
        dff = pd.DataFrame.from_records(shelter.findDocs({'animal_type': 'Dog'})).drop(('_id'), axis=1)
        dff = dff.loc[(dff['breed'] == 'Labrador Retriever Mix') |
                    (dff['breed'] == 'Chesapeake Bay Retriever') |
                    (dff['breed'] == 'Newfoundland')]
        dff = dff.loc[(dff['sex_upon_outcome'] == 'Intact Female')]
        dff = dff.query('26 <= age_upon_outcome_in_weeks <= 156')

    elif radio_options == 'Filter by Mountain Rescue':
        dff = pd.DataFrame.from_records(shelter.findDocs({'animal_type': 'Dog'})).drop(('_id'), axis=1)
        dff = dff.loc[(dff['breed'] == 'German Shepherd') |
                    (dff['breed'] == 'Alaskan Malamute') |
                    (dff['breed'] == 'Old English Sheepdog') |
                    (dff['breed'] == 'Siberian Husky') |
                    (dff['breed'] == 'Rottweiler')]
        dff = dff.loc[(dff['sex_upon_outcome'] == 'Intact Male')]
        dff = dff.query('26 <= age_upon_outcome_in_weeks <= 156')

    elif radio_options == 'Filter by Disaster Rescue':
        dff = pd.DataFrame.from_records(shelter.findDocs({})).drop(('_id'), axis=1)
        dff = dff.loc[(dff['breed'] == 'German Shepherd') |
                    (dff['breed'] == 'Doberman Pinscher') |
                    (dff['breed'] == 'Golden Retriever') |
                    (dff['breed'] == 'Bloodhound') |
                    (dff['breed'] == 'Rottweiler')]
        dff = dff.loc[(dff['sex_upon_outcome'] == 'Intact Male')]
        dff = dff.query('20 <= age_upon_outcome_in_weeks <= 300')

    else:
        dff = pd.DataFrame.from_records(shelter.findDocs({})).drop(('_id'), axis=1)

    return dff.to_dict('records')


@app.callback(
    Output('graph-id', "figure"),
    [Input('datatable-interactivity', "derived_virtual_data")])
def update_graph(allData):
    """ functionality for pie chart """
    dff = pd.DataFrame(allData)

    piechart = px.pie(
        data_frame=dff,
        # pie chart measures outcome_type column
        names=dff['outcome_type'],
        hole=.3,
    )
    return piechart


@app.callback(
    Output('map-id', "children"),
    [Input('datatable-interactivity', "derived_viewport_data"),
    Input('datatable-interactivity', 'derived_virtual_selected_rows')])
def update_map(viewData, derived_virtual_selected_rows):
#FIXME Add in the code for your geolocation chart
    dff = pd.DataFrame.from_dict(viewData)
    
    selected_animal = None

    # if there are no selected rows yet, map default displays first animal of table's current page
    if not derived_virtual_selected_rows:
        selected_animal = dff.iloc[0]
    # else there is a selected row, map displays that animal
    else:
        selected_animal = dff.iloc[derived_virtual_selected_rows[0]]

    animal_breed = selected_animal[3]
    animal_name = selected_animal[8]
    animal_sex = selected_animal[10]

    
    # Austin TX is at [30.75,-97.48]
    return [
        dl.Map(style={'width': '1000px', 'height': '500px'}, center=[30.75,-97.48], zoom=10, children=[
            dl.TileLayer(id="base-layer-id"),
            # Marker with tool tip and popup
            dl.Marker(position=[dff.loc[0,'location_lat'],dff.loc[0,'location_long']], children=[
                dl.Tooltip(animal_breed),
                dl.Popup([
                    html.H1("Animal Name"),
                    html.P(animal_name + " ==> " + animal_sex)
                ])
            ]) 
        ])
    ]

# run the application
if __name__ == '__main__':
    app.run_server(debug=True)