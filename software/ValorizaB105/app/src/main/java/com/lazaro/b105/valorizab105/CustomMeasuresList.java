package com.lazaro.b105.valorizab105;

import android.app.Activity;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.TextView;

public class CustomMeasuresList extends ArrayAdapter<String> {

    private final Activity context;
    private final String[] measures;
    private final String[] dates;
    private int measure_type;

    public CustomMeasuresList(Activity context, String[] measures, String[] dates, int measure_type) {
        super(context, R.layout.listview_measures, measures);

        this.context = context;
        this.measures = measures;
        this.dates = dates;
        this.measure_type = measure_type;
    }

    public View getView(int position, View view, ViewGroup parent) {
        LayoutInflater inflater=context.getLayoutInflater();
        View rowView = inflater.inflate(R.layout.listview_measures, null,true);

        TextView txtTitle = (TextView) rowView.findViewById(R.id.listview_measures_value);
        TextView extratxt = (TextView) rowView.findViewById(R.id.listview_measures_date);
        String title_text;

        switch (measure_type){
            case 1:
                try {
                    if (Float.parseFloat(measures[position]) >= MainActivity.ht_up_th || Float.parseFloat(measures[position]) <= MainActivity.ht_bt_th) {
                        txtTitle.setTextColor(getContext().getResources().getColor(R.color.red_unconnected));
                    }

                }catch (Exception e){}
                if(measures[position].matches(getContext().getResources().getString(R.string.empty_register))){
                    txtTitle.setText(measures[position]);
                } else {
                    title_text = measures[position] + " " + getContext().getResources().getString(R.string.degrees);
                    txtTitle.setText(title_text);
                }
                break;
            case 2:
                try {
                    if (Float.parseFloat(measures[position]) <= MainActivity.po_bt_th) {
                        txtTitle.setTextColor(getContext().getResources().getColor(R.color.red_unconnected));
                    }
                }catch (Exception e){}
                if(measures[position].matches(getContext().getResources().getString(R.string.empty_register))){
                    txtTitle.setText(measures[position]);
                } else {
                    title_text = measures[position] + " " + getContext().getResources().getString(R.string.percetage);
                    txtTitle.setText(title_text);
                }
                break;
            case 3:
                try {
                    String hr_pat = measures[position];
                    String[] p1 = hr_pat.split("/");
                    float hr_from_patient = Float.parseFloat(p1[0]);
                    float var_from_patient = Float.parseFloat(p1[1]);
                    if (hr_from_patient >= MainActivity.hr_up_th || hr_from_patient <= MainActivity.hr_bt_th) {
                        txtTitle.setTextColor(getContext().getResources().getColor(R.color.red_unconnected));
                    }
                    if (var_from_patient >= MainActivity.hr_var_up_th) {
                        txtTitle.setTextColor(getContext().getResources().getColor(R.color.red_unconnected));
                    }
                } catch (Exception e){}
                if(measures[position].matches(getContext().getResources().getString(R.string.empty_register))){
                    txtTitle.setText(measures[position]);
                } else {
                    title_text = measures[position] + " " +getContext().getResources().getString(R.string.bpm_ms);
                    txtTitle.setText(title_text);
                }
                break;
            case 4:
                txtTitle.setText(measures[position]);
                break;
            default:
                break;
        }
        extratxt.setText(dates[position]);
        return rowView;

    };
}
