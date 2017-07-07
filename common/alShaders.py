import pymel.core as pm
from mtoa.ui.ae.shaderTemplate import ShaderAETemplate

UI = {
    'float': pm.attrFieldSliderGrp,
    'rgb': pm.attrColorSliderGrp,
}

class Param:
    name = ''
    label = ''
    annotation = ''
    ptype = ''
    presets = None
    precision = 4

    def __init__(self, n, l, a, p, presets=None, precision=4):
        self.name = n
        self.label = l
        self.annotation = a
        self.ptype = p
        self.presets = presets
        self.precision = precision

def setPresetFlt(ctrl, value):
    attr = pm.attrFieldSliderGrp(ctrl, query=True, attribute=True)
    pm.setAttr(attr, value)

def setPresetRgb(ctrl, value):
    attr = pm.attrColorSliderGrp(ctrl, query=True, attribute=True)
    pm.setAttr(attr + 'R', value[0])
    pm.setAttr(attr + 'G', value[1])
    pm.setAttr(attr + 'B', value[2])

class alShadersTemplate(ShaderAETemplate):

    def customCreateFlt(self, attr):
        # print "creating %s" % attr
        pname = attr.split('.')[-1]
        ptype = self.params[pname].ptype
        plabel = self.params[pname].label
        pann = self.params[pname].annotation
        presets = self.params[pname].presets
        precision = self.params[pname].precision
        controlName = pname + 'Ctrl'
        l = plabel
        if presets is not None:
            l += ' <span>&#8801;</span>' # fix unicode problem in Windows using html
        pm.attrFieldSliderGrp(controlName, attribute=attr, label=l, annotation=pann, precision=precision)
        if presets is not None:
            # pm.attrFieldSliderGrp(controlName, edit=True)
            # pm.popupMenu()
            # for k in sorted(presets, key=presets.get):
            #     pm.menuItem(label=k, command=pm.Callback(setPresetFlt, controlName, presets[k]))
            attrChildren = pm.layout(controlName, query=True, childArray=True)
            pm.popupMenu(button=1, parent=attrChildren[0])
            for k in sorted(presets, key=presets.get):
                pm.menuItem(label=k, command=pm.Callback(setPresetFlt, controlName, presets[k]))


    def customUpdateFlt(self, attr):
        # print "updating attr %s" % attr
        pname = attr.split('.')[-1]
        ptype = self.params[pname].ptype
        controlName = pname + 'Ctrl'
        pm.attrFieldSliderGrp(controlName, edit=True, attribute=attr)

    def customCreateRgb(self, attr):
        pname = attr.split('.')[-1]
        ptype = self.params[pname].ptype
        plabel = self.params[pname].label
        pann = self.params[pname].annotation
        presets = self.params[pname].presets
        controlName = pname + 'Ctrl'

        l = plabel
        if presets is not None:
            l += ' <span>&#8801;</span>' # fix unicode problem in Windows using html
        pm.attrColorSliderGrp(controlName, attribute=attr, label=l, annotation=pann)
        if presets is not None:
            # pm.attrColorSliderGrp(controlName, edit=True)
            # pm.popupMenu()
            attrChildren = pm.layout(controlName, query=True, childArray=True)
            pm.popupMenu(button=1, parent=attrChildren[0])
            for k in sorted(presets):
                pm.menuItem(label=k, command=pm.Callback(setPresetRgb, controlName, presets[k]))

    def customUpdateRgb(self, attr):
        pname = attr.split('.')[-1]
        ptype = self.params[pname].ptype
        controlName = pname + 'Ctrl'
        pm.attrColorSliderGrp(controlName, edit=True, attribute=attr)

    def addCustomFlt(self, param):
        self.addCustom(param, self.customCreateFlt, self.customUpdateFlt)

    def addCustomRgb(self, param):
        self.addCustom(param, self.customCreateRgb, self.customUpdateRgb)

    def addRemapControls(self):
        self.beginLayout('Remap', collapse=True)
        self.addControl('RMPinputMin', label='Input min')
        self.addControl('RMPinputMax', label='Input max')

        self.beginLayout('Contrast', collapse=False)
        self.addControl('RMPcontrast', label='Contrast')
        self.addControl('RMPcontrastPivot', label='Pivot')
        self.endLayout() # end Contrast

        self.beginLayout('Bias and gain', collapse=False)
        self.addControl('RMPbias', label='Bias')
        self.addControl('RMPgain', label='Gain')
        self.endLayout() # end Bias and gain

        self.addControl('RMPoutputMin', label='Output min')
        self.addControl('RMPoutputMax', label='Output max')

        self.beginLayout('Clamp', collapse=False)
        self.addControl('RMPclampEnable', label='Enable')
        self.addControl('RMPthreshold', label='Expand')
        self.addControl('RMPclampMin', label='Min')
        self.addControl('RMPclampMax', label='Max')
        self.endLayout() # end Clamp

        self.endLayout() # end Remap
