#!/usr/bin/python
import sys
import yaml

# get the list of model names from a model yaml file
def get_model_names(model_file_path):
        model_names = []
        with open(model_file_path, "r") as stream:
                try:
                        models_data = yaml.safe_load(stream)
                        # go through the list to get model names
                        model_blobs = models_data["required_models"]
                        for model_blob in model_blobs:
                                model_name = model_blob["model_name"]
                                model_names.append(model_name)
                except yaml.YAMLError as e:
                        print(e)
        return model_names


# main program starts here
if len(sys.argv) != 3:
        print("please input two models.yaml files to compare")
        exit(-1)

models_left = get_model_names(sys.argv[1])
models_right = get_model_names(sys.argv[2])

# get the diff
print "models only exist in " + sys.argv[1] + ": " + str(list(set(models_left) - set(models_right)))
print "models only exist in " + sys.argv[2] + ": " + str(list(set(models_right) - set (models_left)))
