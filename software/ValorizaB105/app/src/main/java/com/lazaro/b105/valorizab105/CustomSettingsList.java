package com.lazaro.b105.valorizab105;

import android.app.Activity;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.TextView;

public class CustomSettingsList extends ArrayAdapter<String> {

    private final Activity context;
    private final String[] names;
    private final String[] values;

    public CustomSettingsList(Activity context, String[] names, String[] values) {
        super(context, R.layout.listview_with_image, names);

        this.context = context;
        this.names = names;
        this.values = values;
    }

    public View getView(int position, View view, ViewGroup parent) {
        LayoutInflater inflater=context.getLayoutInflater();
        View rowView=inflater.inflate(R.layout.listview_settings, null,true);

        TextView txtTitle = (TextView) rowView.findViewById(R.id.listview_setting_name);
        TextView extratxt = (TextView) rowView.findViewById(R.id.listview_settings_value);

        txtTitle.setText(names[position]);
        extratxt.setText(values[position]);
        return rowView;

    };
}
