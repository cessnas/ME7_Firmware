package com.rusefi.tune.xml;

import javax.xml.bind.annotation.XmlAttribute;
import java.util.ArrayList;
import java.util.List;

public class Page {
    public final List<Constant> constant = new ArrayList<>();
    public final List<PcVariable> pcVariable = new ArrayList<>();

    private Integer number;
    private Integer size;

    public Page() {
    }

    public Page(Integer number, Integer size) {
        this.number = number;
        this.size = size;
    }

    @XmlAttribute
    public Integer getNumber() {
        return number;
    }

    public void setNumber(Integer number) {
        this.number = number;
    }

    @XmlAttribute
    public Integer getSize() {
        return size;
    }

    public void setSize(Integer size) {
        this.size = size;
    }

    @Override
    public String toString() {
        return "Page{" +
                "constant.size=" + constant.size() +
                ", number=" + number +
                ", size=" + size +
                '}';
    }
}
