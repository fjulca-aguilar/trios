# coding=utf-8
# coding=utf-8
"""

@author: André V Lopes
"""

import time

import lasagne
import numpy as np
import theano
from sklearn.model_selection import train_test_split


class LasagneClassifier():
    network = None
    batch_size = None
    verbose = None
    minimize = None
    ordered = True
    dtype = None

    def __init__(self, minimize=False, ordered=True, dtype=np.uint8, *a, **kw):
        self.batch_size = kw['batch_size']
        self.verbose = kw['verbose']
        self.network = kw['network']
        self.minimize = minimize
        self.ordered = ordered
        self.dtype = dtype
        self.input_var = kw['input_var']
        self.target_var = kw['target_var']

    def setWeights(self, network, param_values):
        try:
            lasagne.layers.set_all_param_values(network, param_values)
        except Exception as ex:
            print 'Failure to load Weights , Reason :' + str(ex.message)

    def getNetworkWeights(self, network):
        try:
            return lasagne.layers.get_all_param_values(network)
        except Exception as ex:
            print 'Failure to get Weights , Reason :' + str(ex.message)

    def normalize(self, data):
        data = data / 255
        return data

    def denormalize(self, data):
        data = data * 255
        return data

    def minibatch_iterator(self, inputs, targets, batch_size):

        assert len(inputs) == len(targets)

        indices = np.arange(len(inputs))
        np.random.shuffle(indices)

        for start_idx in range(0, len(inputs) - batch_size + 1, batch_size):
            excerpt = indices[start_idx:start_idx + batch_size]
            yield inputs[excerpt], targets[excerpt]

    def minibatch_iterator_predictor(self, inputs, batch_size):
        for start_idx in range(0, len(inputs) - batch_size + 1, batch_size):
            excerpt = slice(start_idx, start_idx + batch_size)
            yield inputs[excerpt]

    def get_validation_data(self, x, y, validation_percentage=0, random_state=1):
        x, x_val, y, y_val = train_test_split(x, y, test_size=validation_percentage / 100.00, random_state=random_state)

        return x, y, x_val, y_val

    def train(self, dataset, kw):
        max_epochs = kw.get('epochs', 10)
        max_patience = kw.get('max_patience', int((10 * max_epochs) / 100))
        test_loss = kw.get('test_loss')
        test_acc = kw.get('test_acc')
        updates = kw.get('update')
        loss_or_grads = kw.get('loss_or_grads')
        X, Y = dataset
        X_test, Y_test = kw.get('testset', (None, None))
        validation_percentage = kw.get('validation_percentage', 30)
        random_state = kw.get('random_state', 1)

        # Separate a Validation set
        X, Y, X_val, Y_val = self.get_validation_data(x=X, y=Y, validation_percentage=validation_percentage, random_state=random_state)

        # Normalize X,Y by 255
        X = self.normalize(X)
        Y = self.normalize(Y)

        # Normalize the validation set
        X_val = self.normalize(X_val)
        Y_val = self.normalize(Y_val)

        # Normalize X_test and y_test by 255
        X_test = self.normalize(X_test)
        Y_test = self.normalize(Y_test)

        # x = x.reshape((-1, 1, lasagne.layers.get_all_layers(self.network)[0].shape[2], lasagne.layers.get_all_layers(self.network)[0].shape[3]))
        # Reshape them to fit the Convnet
        X = X.reshape((-1, 1, lasagne.layers.get_all_layers(self.network)[0].shape[2], lasagne.layers.get_all_layers(self.network)[0].shape[3]))
        Y = Y.reshape(-1, 1)

        # Reshape the test set
        X_test = X_test.reshape((-1, 1, lasagne.layers.get_all_layers(self.network)[0].shape[2], lasagne.layers.get_all_layers(self.network)[0].shape[3]))
        Y_test = Y_test.reshape(-1, 1)

        # Reshape the validation set
        X_val = X_val.reshape((-1, 1, lasagne.layers.get_all_layers(self.network)[0].shape[2], lasagne.layers.get_all_layers(self.network)[0].shape[3]))
        Y_val = Y_val.reshape(-1, 1)

        # Create the theano functions that will run on the GPU/CPU
        train_fn = theano.function([self.input_var, self.target_var], loss_or_grads, updates=updates)
        val_fn = theano.function([self.input_var, self.target_var], [test_loss, test_acc])

        # Save epochs in which nan's ocurred.
        nan_occurrences = []

        # Prepare the patience variables
        if max_patience != -1:

            # Do not allow max_patience to be 0. It can be -1 to disable patience, but not 0.
            if max_patience == 0:
                max_patience = 1

            current_patience = int(max_patience)
            best_validation_loss = float('inf')

            best_acc = -1
            best_weights = None

        # Finally, launch the training loop.
        print("Starting training...")

        for epoch in range(max_epochs):
            # In each epoch, do a full pass over the training data:
            train_err = 0
            train_batches = 0
            start_time = time.time()
            for batch in self.minibatch_iterator(X, Y, self.batch_size):
                inputs, targets = batch
                train_err += train_fn(inputs, targets)
                train_batches += 1

            # And a full pass over the validation data:
            val_err = 0
            val_acc = 0
            val_batches = 0
            for batch in self.minibatch_iterator(X_val, Y_val, self.batch_size):
                inputs, targets = batch
                err, acc = val_fn(inputs, targets)
                val_err += err
                val_acc += acc
                val_batches += 1

            #Calculate Results
            trainingLoss = (train_err / train_batches)
            validationLoss = (val_err / val_batches)
            validationAccuracy = (val_acc / val_batches * 100)

            # Then print the results for this epoch:
            print("Epoch {} of {} took {:.3f}s".format(epoch + 1, max_epochs, time.time() - start_time))
            print("  Training Loss:\t\t{:.6f}".format(trainingLoss))
            print("  Validation Loss:\t\t{:.6f}".format(validationLoss))
            print("  Validation Accuracy:\t\t{:.4f} %".format(validationAccuracy))

            ##Check for NAN and append NAN warnings to HTML LOG.
            if np.isnan(trainingLoss):
                nan_occurrences.append("Training Loss is NaN at epoch :" + str(epoch))

            if np.isnan(validationLoss):
                nan_occurrences.append("Validation Loss is NaN at epoch :" + str(epoch))

            if np.isnan(validationAccuracy):
                nan_occurrences.append("Validation Accuracy is NaN at epoch :" + str(epoch))

            # Patience

            # If patience Is ENABLED then go through the patience-logic
            if max_patience != -1:

                if epoch == 0 or best_validation_loss == -1 or best_validation_loss is None or best_acc == -1 or best_acc is None:
                    best_validation_loss = validationLoss
                    best_acc = validationAccuracy
                    best_weights = self.getNetworkWeights(self.network)

                    ##First verify when to decrease patience count. Decreasing means losing patience.
                if epoch >= 1:

                    # If validation loss is WORSE than best loss, LOSE patience
                    if validationLoss > best_validation_loss:
                        current_patience = current_patience - 1

                    else:
                        # if the current loss is BETTER than best, RESET patience
                        if validationLoss <= best_validation_loss:
                            best_validation_loss = validationLoss
                            best_acc = validationAccuracy
                            current_patience = int(max_patience)
                            best_weights = self.getNetworkWeights(self.network)


                    # Check if theres no more patience and if its time to stop.
                    if current_patience <= 0:
                        print "\nNo more patience. Stopping training...\n"
                        if best_weights != None:
                            self.setWeights(network=self.network, param_values=best_weights)
                        break


                print "  Current Patience : " + str(current_patience) + " | Max Patience : " + str(max_patience) + "\n"



        # After training, we compute and print the test error:
        test_err = 0
        test_acc = 0
        test_batches = 0
        for batch in self.minibatch_iterator(X_test, Y_test, self.batch_size):
            inputs, targets = batch
            err, acc = val_fn(inputs, targets)
            test_err += err
            test_acc += acc
            test_batches += 1

        print("Final results:")
        print("  Test Loss:\t\t\t{:.6f}".format(test_err / test_batches))
        print("  Test Accuracy:\t\t{:.4f} %".format(test_acc / test_batches * 100))

        # Print nan_occurences
        for occurrence in nan_occurrences:
            print(str(occurrence))

        score = [(test_err / test_batches), (test_acc / test_batches * 100)]

        return score

    ########################

    def apply(self, x):
        # Normalize value to 0~1
        x = self.normalize(x)

        # Reshape so it fits the network input
        x = x.reshape((-1, 1, self.network.input_layer.shape[2], self.network.input_layer.shape[3]))

        # Predict
        predict_function = theano.function([self.input_var], lasagne.layers.get_output(self.network, deterministic=True))

        prediction = np.empty((x.shape[0], 1), dtype=np.float32)

        index = 0
        for batch in self.minibatch_iterator_predictor(inputs=x, batch_size=self.batch_size):
            inputs = batch
            y = predict_function(inputs)

            prediction[index * self.batch_size:self.batch_size * (index + 1)] = y
            index += 1

        # Reshape properly
        prediction = prediction.reshape(-1)

        # Return to 0-255 range
        prediction = self.denormalize(prediction)

        return prediction

    def apply_batch(self, x):
        # Normalize value to 0~1
        x = self.normalize(x)

        # Reshape so it fits the network input
        x = x.reshape((-1, 1, lasagne.layers.get_all_layers(self.network)[0].shape[2], lasagne.layers.get_all_layers(self.network)[0].shape[3]))

        # Predict

        predict_function = theano.function([self.input_var], lasagne.layers.get_output(self.network, deterministic=True))

        prediction = np.empty((x.shape[0], 1), dtype=np.float32)

        index = 0
        for batch in self.minibatch_iterator_predictor(inputs=x, batch_size=self.batch_size):
            inputs = batch
            y = predict_function(inputs)

            prediction[index * self.batch_size:self.batch_size * (index + 1)] = y
            index += 1

        # Reshape properly
        prediction = prediction.reshape(-1)

        # Return to 0-255 range
        prediction = self.denormalize(prediction)

        return prediction

    def write_state(self, obj_dict={}):

        save_to = str(obj_dict['save_weights_as'])

        np.savez(save_to, *lasagne.layers.get_all_param_values(self.network))

    def set_state(self, obj_dict={}):

        try:
            load_from = str(obj_dict['load_weights_as'])

            with np.load(load_from) as f:
                param_values = [f['arr_%d' % i] for i in range(len(f.files))]

            lasagne.layers.set_all_param_values(self.network, param_values)
        except Exception as ex:
            print('Error when attempting to load Weights , Reason :', ex.message)

        return self.network